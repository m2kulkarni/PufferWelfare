"""Complete simultaneous-order interface to the C Welfare Diplomacy adjudicator.

The older Discrete(1088) environment is a restricted integration interface.
Game accepts a full list of textual orders for every power in each phase.
"""
from collections.abc import Mapping
import copy
import hashlib
import json
from pathlib import Path
import random

from .diplomacy import Diplomacy
from . import binding

POWERS = ('AUSTRIA', 'ENGLAND', 'FRANCE', 'GERMANY', 'ITALY', 'RUSSIA', 'TURKEY')
PHASES = ('SPRING_MOVEMENT', 'SPRING_RETREAT', 'FALL_MOVEMENT', 'FALL_RETREAT',
          'WINTER_ADJUSTMENT', 'COMPLETED')


class Game:
    """Seven powers submit orders together; omitted units hold or disband on retreat.

    Winter builds may be waived by submitting an empty list. Welfare scores are
    accumulated after Winter. No press, negotiation or hidden information layer
    is imposed. `observe(power)` returns the complete public state and actor.
    """
    def __init__(self, max_years=10, welfare_mode=True, seed=1):
        self.config = dict(max_years=max_years, welfare_mode=welfare_mode, seed=seed)
        self._env = Diplomacy(**self.config)
        self._history = []
        self.reset()

    def _open(self):
        self._env._require_open()

    @property
    def state(self):
        self._open()
        raw = binding.query_game_state(self._env.env_handle)
        locations = binding.query_map_info(self._env.env_handle)['locations']
        names = [loc['name'] for loc in locations]
        powers = {}
        for name, power in zip(POWERS, raw['powers']):
            powers[name] = {'units': [f"{'A' if u['type'] == 1 else 'F'} {names[u['location']]}"
                                      for u in power['units']],
                            'centers': [names[c] for c in power['centers']],
                            'welfare': power['welfare_points']}
        retreats = []
        for d in binding.get_dislodged_units(self._env.env_handle):
            retreats.append({'power': POWERS[d['power_id']],
                             'unit': f"{'A' if d['type'] == 1 else 'F'} {names[d['from_location']]}",
                             'attacker_origin': names[d['dislodged_by_location']],
                             'destinations': [names[i] for i in d['possible_retreats']]})
        return {'year': raw['year'], 'phase': PHASES[raw['phase']],
                'terminated': raw['is_game_over'], 'powers': powers,
                'locations': locations, 'retreats': retreats}

    def observe(self, power):
        if power not in POWERS:
            raise ValueError(f'unknown power: {power}')
        state = self.state
        state['actor'] = power
        state['unit_slots'] = list(state['powers'][power]['units'])
        return state

    def observations(self):
        state = self.state
        return {p: dict(copy.deepcopy(state), actor=p,
                        unit_slots=list(state['powers'][p]['units'])) for p in POWERS}

    def reset(self, seed=None):
        self._env.reset(seed=seed)
        if seed is not None:
            self.config['seed'] = seed
        self._history = []
        return self.observations(), {'state_hash': self.state_hash()}

    def state_hash(self):
        return hashlib.sha256(json.dumps(self.state, sort_keys=True).encode()).hexdigest()

    def step(self, orders):
        self._open()
        if self.state['terminated']:
            raise RuntimeError('game is complete; reset before stepping')
        if not isinstance(orders, Mapping) or any(p not in POWERS for p in orders):
            raise ValueError('orders must map power names to lists of order strings')
        normalized = {}
        initial_state = self.state
        phase = initial_state['phase']
        locations = initial_state['locations']
        # Validate the entire input before touching any power's pending orders.
        for p in POWERS:
            values = orders.get(p, [])
            if not isinstance(values, (list, tuple)) or len(values) > 34:
                raise ValueError('each power must supply at most 34 order strings')
            values = [v.strip().upper() if isinstance(v, str) else v for v in values]
            parsed = []
            for value in values:
                if not isinstance(value, str) or len(value.encode()) >= 64:
                    raise ValueError('orders must be strings shorter than 64 bytes')
                order = binding.test_parse_order(self._env.env_handle, value)
                if order is None:
                    raise ValueError(f'cannot parse order: {value}')
                parsed.append(order)
            kinds = {o['type'] for o in parsed}
            if phase == 'WINTER_ADJUSTMENT' and 6 in kinds and 7 in kinds:
                raise ValueError('a Winter order set cannot mix builds and disbands')
            origins = [locations[o['unit_location']]['parent'] if
                       locations[o['unit_location']]['parent'] >= 0 else o['unit_location']
                       for o in parsed]
            if len(origins) != len(set(origins)):
                raise ValueError('submit only one order per province/unit')
            normalized[p] = values
        for p in POWERS:
            binding.game_submit_orders(self._env.env_handle, POWERS.index(p), normalized[p])
        _, rewards, terminals, truncations, _ = self._env.step([0] * 7)
        info = {'orders': normalized, 'results': {}, 'state_hash': self.state_hash()}
        for p in POWERS:
            idx = POWERS.index(p)
            info['results'][p] = [binding.get_order_result(self._env.env_handle, idx, i)
                                  for i in range(binding.get_num_orders(self._env.env_handle, idx))]
        self._history.append(copy.deepcopy(normalized))
        return (self.observations(), dict(zip(POWERS, map(float, rewards))),
                bool(terminals.all()), bool(truncations.any()), info)

    def possible_orders(self, power):
        """Locally valid candidates by unit/location; sets still need coordination.

        A support or convoy candidate is not a promise it will succeed: other
        powers' simultaneous orders determine that. Winter sets must obey the
        build count and cannot mix builds with disbands.
        """
        state = self.observe(power)
        locs = state['locations']
        by_name = {loc['name']: loc for loc in locs}
        phase = state['phase']
        own = state['powers'][power]
        handle = self._env.env_handle
        pidx = POWERS.index(power)
        output = {}
        def parent(name):
            loc = by_name[name]
            return locs[loc['parent']]['name'] if loc['parent'] >= 0 else name
        if phase == 'COMPLETED':
            return output
        if 'RETREAT' in phase:
            for retreat in state['retreats']:
                if retreat['power'] == power:
                    unit = retreat['unit']
                    output[unit] = [unit + ' D'] + [unit + ' R ' + d for d in retreat['destinations']]
            return output
        if phase == 'WINTER_ADJUSTMENT':
            if self.config['welfare_mode'] or len(own['units']) > len(own['centers']):
                output.update({u: [u + ' D'] for u in own['units']})
            if len(own['centers']) > len(own['units']):
                occupied = {parent(u.split()[1]) for q in state['powers'].values() for u in q['units']}
                for loc in locs:
                    base = parent(loc['name'])
                    base_loc = by_name[base]
                    if (base_loc['home_power'] == pidx and base in own['centers'] and base not in occupied):
                        values = []
                        if loc['parent'] < 0 and loc['type'] != 2:
                            values.append('A ' + loc['name'] + ' B')
                        if loc['type'] == 1:
                            values.append('F ' + loc['name'] + ' B')
                        if values:
                            output[loc['name']] = values
            return output
        units = [u for q in state['powers'].values() for u in q['units']]
        for unit in own['units']:
            kind, origin = unit.split()
            candidates = {unit + ' H'}
            for dest in by_name:
                if dest == 'SWI' or (kind == 'A' and '/' in dest):
                    continue
                if binding.can_move_names(handle, 1 if kind == 'A' else 2, origin, dest):
                    candidates.add(unit + ' - ' + dest)
                if kind == 'A' and by_name[dest]['parent'] < 0 and by_name[dest]['type'] in (1, 3):
                    candidate = unit + ' - ' + dest + ' VIA'
                    if binding.test_validate_order(handle, pidx, candidate) == 0:
                        candidates.add(candidate)
            for target in units:
                if target == unit:
                    continue
                candidate = unit + ' S ' + target
                if binding.test_validate_order(handle, pidx, candidate) == 0:
                    candidates.add(candidate)
                for dest in by_name:
                    if dest == 'SWI':
                        continue
                    candidate = unit + ' S ' + target + ' - ' + dest
                    if binding.test_validate_order(handle, pidx, candidate) == 0:
                        candidates.add(candidate)
                    if kind == 'F' and by_name[origin]['type'] == 2 and target.startswith('A '):
                        candidate = unit + ' C ' + target + ' - ' + dest
                        if binding.test_validate_order(handle, pidx, candidate) == 0:
                            candidates.add(candidate)
            output[unit] = sorted(candidates)
        return output

    def scripted_orders(self, seed=0):
        """A deterministic legal-order example policy, not a strong opponent."""
        rng = random.Random(seed)
        state = self.state
        result = {}
        for p, data in state['powers'].items():
            options = self.possible_orders(p)
            if state['phase'] == 'WINTER_ADJUSTMENT':
                # Waive builds; disband excess in standard mode, one in welfare.
                excess = max(0, len(data['units']) - len(data['centers']))
                count = max(excess, min(1, len(data['units']))) if self.config['welfare_mode'] else excess
                result[p] = [u + ' D' for u in data['units'][:count]]
            else:
                chosen = []
                for values in options.values():
                    if not values:
                        continue
                    moves = [v for v in values if ' - ' in v and ' S ' not in v and ' C ' not in v]
                    chosen.append(rng.choice(moves if moves and rng.random() < 0.65 else values))
                result[p] = chosen
        return result

    def save_replay(self, path):
        Path(path).write_text(json.dumps({'format': 1, 'config': self.config,
                                         'orders': self._history, 'state_hash': self.state_hash()}, indent=2))

    @classmethod
    def from_replay(cls, path):
        data = json.loads(Path(path).read_text())
        if data['format'] != 1:
            raise ValueError('unsupported replay format')
        game = cls(**data['config'])
        try:
            for orders in data['orders']:
                game.step(orders)
            if game.state_hash() != data['state_hash']:
                raise ValueError('replay state mismatch')
            return game
        except BaseException:
            game.close()
            raise

    def render(self):
        state = self.state
        return '\n'.join([f"{state['year']} {state['phase']}"] +
                         [f"{p}: {', '.join(v['units'])}; centers={len(v['centers'])}; welfare={v['welfare']}"
                          for p, v in state['powers'].items()])

    def close(self):
        self._env.close()

    def __enter__(self):
        return self

    def __exit__(self, *args):
        self.close()
