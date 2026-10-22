# Complete-game interface

`pufferlib.ocean.diplomacy.Game` is the public simultaneous-order simulator.
It uses the existing C adjudicator, not a second Python implementation. A step
accepts `{POWER: [order, ...]}` for all seven powers. Missing movement orders
hold, missing retreats disband, and empty Winter lists waive builds. Supply
centers transfer after Fall movement/retreats. Welfare is accumulated only after
Winter, following paper section 2.2 and the local rule-source priority.

Observations are complete structured public state, including all 82 internal map
entries, actor identity, exact unit slots, owners, homes, and dislodged units with
retreat destinations. This avoids silently changing the older numeric ABI.
`possible_orders(power)` returns local candidates. Simultaneous supports and
convoys can fail depending on other orders; Winter order sets still need to obey
build counts and choose either builds or disbands. Syntax errors and duplicate
unit/province orders are rejected before any pending orders are modified. Other
illegal game orders are adjudicated as VOID; they are not Python exceptions.

The legacy `Diplomacy` / PufferLib 4 CPU `Discrete(1088)` interface is retained
for compatibility and trainer integration tests. It selects one unit per power
and has incomplete order expressivity and observations. It is not the complete
research action interface. Do not label its MLP smoke a learned baseline.

Game API:

```python
from pufferlib.ocean.diplomacy import Game
with Game(max_years=10, seed=42) as game:
    while not game.state['terminated']:
        orders = game.scripted_orders(seed=game.state['year'])
        observations, rewards, terminated, truncated, info = game.step(orders)
    print(game.render())
    game.save_replay('game.json')
with Game.from_replay('game.json') as restored:
    print(restored.state_hash())
```

Replays record initial configuration and complete submitted orders, then verify
the final state hash on loading. They do not capture private changes made through
raw test/debug bindings. No press or negotiation layer is supplied. The example
scripted policy demonstrates the interface and is not a strong opponent.

## Verification and test corrections

The 46 previously skipped cases were audited. Twenty-six were empty `pass`
placeholders; others assumed an obsolete dictionary observation space, incorrectly
expected three Russian units, or exercised a no-op renderer. They did not provide
coverage. Their intended behaviors now have executable checks in the C and public
Game suites. The three previously skipped support/circular tests were enabled
unchanged after independent reproduction. Existing DATC expectations were not
changed. The numeric ABI tests retain their existing shapes.

The unchanged pinned `standard.map` fixture (reference commit
60520aaa4661e4c95abfd30ce6483990bb127567) verifies every direct army base-province
and fleet edge. Additional regressions cover actual Fall capture/reset, legal
home/coast builds, support reachability to destination, convoy endpoints, typed
disbands and retreats, occupied retreat destinations, opposite-coast retreat
conflicts, result mapping with reversed order submission, mixed adjustment sets,
complete seeded games and replay. Passing tests are evidence for these cases,
not a proof of all Diplomacy paradoxes or performance at scale.
