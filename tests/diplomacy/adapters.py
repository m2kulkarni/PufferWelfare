from typing import Any, Dict, List, Optional
import numpy as np
from pufferlib.ocean.diplomacy import Diplomacy, binding


class _History:
    def __init__(self):
        self._values = []

    def add(self, value):
        self._values.append(value)

    def last_value(self):
        return self._values[-1] if self._values else {}

    def first_value(self):
        return self._values[0] if self._values else {}

    def values(self):
        return list(self._values)


class GameAdapter:
    def __init__(self, welfare_mode: bool = True, max_years: int = 10):
        self.env = Diplomacy(welfare_mode=welfare_mode, max_years=max_years)
        self._pending_orders = {}
        self._popped_units = set()
        self.result_history = _History()
        self.order_history = _History()
        self.state_history = _History()
        self.message_history = _History()
        self._reset()

    def _reset(self):
        self.obs, self.info = self.env.reset()
        self._popped_units = set()

    def clear_units(self):
        for p in range(7):
            binding.game_clear_units(self.env.env_handle, p)

    def clear_centers(self):
        binding.game_clear_centers(self.env.env_handle)

    def set_units(self, power: str, units):
        power_idx = ["AUSTRIA","ENGLAND","FRANCE","GERMANY","ITALY","RUSSIA","TURKEY"].index(power.upper())
        if isinstance(units, str):
            units = [units] if units.strip() else []
        elif not isinstance(units, list):
            units = list(units) if units else []

        norm = []
        for u in units:
            if isinstance(u, str):
                parts = u.strip().split()
                if len(parts) >= 2:
                    norm.append((parts[0].upper(), parts[1].upper()))
            elif isinstance(u, (list, tuple)) and len(u) >= 2:
                norm.append((str(u[0]).upper(), str(u[1]).upper()))
        binding.game_set_units(self.env.env_handle, power_idx, norm)

    def set_centers(self, power: str, centers):
        power_idx = ["AUSTRIA","ENGLAND","FRANCE","GERMANY","ITALY","RUSSIA","TURKEY"].index(power.upper())
        if isinstance(centers, str):
            centers = [centers]
        centers = [c.upper() for c in centers]
        binding.game_set_centers(self.env.env_handle, power_idx, centers)

    def get_phase(self) -> str:
        state = binding.query_game_state(self.env.env_handle)
        phases = {0: "SPRING MOVEMENT", 1: "SPRING RETREAT", 2: "FALL MOVEMENT",
                  3: "FALL RETREAT", 4: "WINTER ADJUSTMENT", 5: "COMPLETED"}
        return phases.get(state["phase"], "UNKNOWN")

    def get_year(self) -> int:
        return binding.query_game_state(self.env.env_handle)["year"]

    def get_current_phase(self):
        return self.get_phase()

    @property
    def phase_type(self):
        phase = binding.query_game_state(self.env.env_handle)["phase"]
        if phase in (0, 2): return 'M'
        if phase in (1, 3): return 'R'
        if phase == 4: return 'A'
        return 'X'

    def set_current_phase(self, phase_str: str):
        pass

    def get_all_possible_orders(self) -> Dict[str, List[str]]:
        raise NotImplementedError("Order generation not yet implemented in C")

    def set_orders(self, power: str, orders) -> None:
        if isinstance(orders, str):
            orders = [orders] if orders.strip() else []
        elif not isinstance(orders, list):
            orders = list(orders) if orders else []

        if not hasattr(self, "_pending_orders"):
            self._pending_orders = {}
        self._pending_orders[power] = orders

        snapshot = self.order_history.last_value().copy() if self.order_history.values() else {}
        snapshot[str(power).upper()] = list(orders)
        self.order_history.add(snapshot)

    def _get_results(self, orders_submitted):
        results = {}
        power_names = ["AUSTRIA","ENGLAND","FRANCE","GERMANY","ITALY","RUSSIA","TURKEY"]

        for pname, orders in orders_submitted.items():
            pidx = power_names.index(pname)
            num_orders = binding.get_num_orders(self.env.env_handle, pidx)

            for order_idx in range(num_orders):
                result_code = binding.get_order_result(self.env.env_handle, pidx, order_idx)

                if order_idx < len(orders):
                    order_str = orders[order_idx]
                    parts = order_str.strip().upper().split()
                    if len(parts) >= 2:
                        unit_key = f"{parts[0]} {parts[1]}"

                        if result_code == 1: results[unit_key] = []
                        elif result_code == 2: results[unit_key] = ['bounce']
                        elif result_code == 3: results[unit_key] = ['cut']
                        elif result_code == 4: results[unit_key] = ['dislodged']
                        elif result_code == 5: results[unit_key] = ['void']
                        elif result_code == 6: results[unit_key] = ['bounce']
                        else: results[unit_key] = []

        return results

    def process(self) -> None:
        state = binding.query_game_state(self.env.env_handle)
        phase = state["phase"]
        power_names = ["AUSTRIA","ENGLAND","FRANCE","GERMANY","ITALY","RUSSIA","TURKEY"]

        orders_submitted = {}
        if hasattr(self, "_pending_orders") and self._pending_orders:
            for power, orders in self._pending_orders.items():
                if power is None:
                    continue
                pname = str(power).upper()
                if pname not in power_names:
                    continue
                pidx = power_names.index(pname)
                binding.game_submit_orders(self.env.env_handle, pidx, orders)
                orders_submitted[pname] = orders
            self._pending_orders = {}

        actions = np.zeros(7, dtype=np.int32)
        self.obs, rewards, dones, truncated, self.info = self.env.step(actions)

        if orders_submitted:
            results = self._get_results(orders_submitted)
            self.result_history.add(results)

            for unit_key, result in results.items():
                if "void" in result or "bounce" in result:
                    self._popped_units.add(unit_key)

        self.state_history.add(binding.query_game_state(self.env.env_handle))

    def _state_fingerprint(self) -> str:
        st = binding.query_game_state(self.env.env_handle)
        parts = [str(st["year"]), str(st["phase"])]
        for p in st["powers"]:
            centers = ','.join(map(str, sorted(p["centers"])))
            units = ','.join(f"{u['type']}@{u['location']}" for u in sorted(p["units"], key=lambda x: (x['type'], x['location'])))
            parts.append(centers)
            parts.append(units)
        return '|'.join(parts)

    def get_hash(self):
        return hash(self._state_fingerprint())

    def rebuild_hash(self):
        return hash(self._state_fingerprint())

    @property
    def current_short_phase(self) -> str:
        st = binding.query_game_state(self.env.env_handle)
        year = st["year"]
        phase = st["phase"]
        season = 'S' if phase in (0, 1) else 'F' if phase in (2, 3) else 'W'
        letter = 'M' if phase in (0, 2) else 'R' if phase in (1, 3) else 'A'
        return f"{season}{year}{letter}"

    def get_centers(self, power: str) -> List[str]:
        if power is None:
            return {name: self.get_centers(name) for name in ["AUSTRIA","ENGLAND","FRANCE","GERMANY","ITALY","RUSSIA","TURKEY"]}

        state = binding.query_game_state(self.env.env_handle)
        power_idx = ["AUSTRIA","ENGLAND","FRANCE","GERMANY","ITALY","RUSSIA","TURKEY"].index(str(power).upper())
        centers_idx = state["powers"][power_idx]["centers"]
        map_info = binding.query_map_info(self.env.env_handle)
        idx_to_name = [loc["name"] for loc in map_info["locations"]]
        return [idx_to_name[i] for i in centers_idx]

    def get_units(self, power=None):
        map_info = binding.query_map_info(self.env.env_handle)
        idx_to_name = [loc["name"] for loc in map_info["locations"]]

        def units_for_power(pname: str):
            state = binding.query_game_state(self.env.env_handle)
            pidx = ["AUSTRIA","ENGLAND","FRANCE","GERMANY","ITALY","RUSSIA","TURKEY"].index(pname)
            units = state["powers"][pidx]["units"]
            return [f"{'A' if u['type'] == 1 else 'F'} {idx_to_name[u['location']]}" for u in units]

        if power is None:
            return {name: units_for_power(name) for name in ["AUSTRIA","ENGLAND","FRANCE","GERMANY","ITALY","RUSSIA","TURKEY"]}
        return units_for_power(str(power).upper())

    def get_welfare_points(self, power: str) -> int:
        state = binding.query_game_state(self.env.env_handle)
        power_idx = ["AUSTRIA","ENGLAND","FRANCE","GERMANY","ITALY","RUSSIA","TURKEY"].index(power.upper())
        return state["powers"][power_idx]["welfare_points"]

    def is_game_over(self) -> bool:
        return binding.query_game_state(self.env.env_handle)["phase"] == 5

    def get_order_status(self, power=None, unit=None):
        results = self.result_history.last_value() if self.result_history.values() else {}

        if unit:
            return results.get(unit, [])
        elif power:
            all_units = self.get_units(power)
            return {u: results.get(u, []) for u in all_units}
        else:
            result = {}
            for pname in ["AUSTRIA","ENGLAND","FRANCE","GERMANY","ITALY","RUSSIA","TURKEY"]:
                units = self.get_units(pname)
                result[pname] = {u: results.get(u, []) for u in units}
            return result

    @property
    def dislodged(self):
        dislodged_info = binding.get_dislodged_units(self.env.env_handle)
        map_info = binding.query_map_info(self.env.env_handle)
        idx_to_name = [loc["name"] for loc in map_info["locations"]]

        result = {}
        for d_info in dislodged_info:
            unit_type = "A" if d_info["type"] == 1 else "F"
            from_loc = idx_to_name[d_info["from_location"]]
            unit_str = f"{unit_type} {from_loc}"
            dislodger_loc = idx_to_name[d_info["dislodged_by_location"]]
            result[unit_str] = dislodger_loc[:3]

        return result

    @property
    def popped(self):
        return self._popped_units

    @property
    def command(self):
        last_orders = self.order_history.last_value() if self.order_history.values() else {}
        result = {}

        for power_name, orders in last_orders.items():
            for order_str in orders:
                parts = order_str.strip().upper().split()
                if len(parts) >= 2:
                    unit_key = f"{parts[0]} {parts[1]}"
                    if len(parts) >= 3:
                        if parts[2] in ['-', '->']: result[unit_key] = '-'
                        elif parts[2] in ['S', 'SUPPORT', 'SUPPORTS']: result[unit_key] = 'S'
                        elif parts[2] in ['C', 'CONVOY', 'CONVOYS']: result[unit_key] = 'C'
                        elif parts[2] in ['H', 'HOLD', 'HOLDS']: result[unit_key] = 'H'
                        else: result[unit_key] = parts[2][0].upper()
                    else:
                        result[unit_key] = 'H'

        return result

    @property
    def ordered_units(self):
        return {pname: self.get_units(pname) for pname in ["AUSTRIA","ENGLAND","FRANCE","GERMANY","ITALY","RUSSIA","TURKEY"]}

    def _unit_owner(self, unit_str: str, coast_required: bool = False):
        unit_str = unit_str.strip().upper()
        parts = unit_str.split()
        if len(parts) < 2:
            return None

        class PowerStub:
            def __init__(self, name):
                self.name = name

        for pname in ["AUSTRIA","ENGLAND","FRANCE","GERMANY","ITALY","RUSSIA","TURKEY"]:
            units = self.get_units(pname)
            for u in units:
                u_normalized = u.strip().upper()
                if coast_required:
                    if u_normalized == unit_str:
                        return PowerStub(pname)
                else:
                    u_base = u_normalized.split('/')[0]
                    unit_base = unit_str.split('/')[0]
                    if u_base == unit_base:
                        return PowerStub(pname)

        return None
