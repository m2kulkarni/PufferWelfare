# Welfare Diplomacy C Port - Status

**Last Updated**: December 31, 2025 (Session 3)
**Current**: 143/160 tests passing (89.4%)
**Previous**: 139/160 (86.9%)
**Session Gain**: +4 tests (convoy disruption, dislodgement coasts)

## Latest Session: Convoy Disruption & Dislodgement (Dec 31, 2025 - Session 3)

**Summary**: Fixed convoy disruption timing for circular movements, failed convoy handling, and dislodgement logic for split-coast locations.

### Fixes Implemented

1. **test_6_c_5**: Convoy disruption before circular movement (DATC 6.C.5)
   - **Issue**: Convoy disruption check happened AFTER cycle detection
   - **Fix**: Added preliminary convoy disruption check before Step 5 (cycle detection)
   - **Impact**: When a convoying fleet is dislodged, the convoy is disrupted BEFORE circular movement resolution
   - **Files**: `diplomacy.c:1680-1740`

2. **test_6_d_8**: Failed convoy can't receive hold support + `is_convoy_possible`
   - **Issue**: Army with failed convoy could receive hold support (treated as holding)
   - **Fix1**: Added `is_convoy_possible()` to validate orders where convoy was geometrically possible
   - **Fix2**: Non-adjacent army moves without convoy fail but order is valid (can't get hold support)
   - **Files**: `diplomacy.c:897-923, 652-656, 1696-1704`

3. **test_6_d_23**: Dislodgement with coast parent comparison
   - **Issue**: Dislodgement logic compared exact locations, not parent locations
   - **Fix**: Attack on SPA/SC now dislodges unit at SPA/NC (same parent)
   - **Files**: `diplomacy.c:2083-2108`

4. **test_6_d_31**: NO_CONVOY result for non-adjacent army without convoy
   - **Issue**: Army moving non-adjacent without convoy got BOUNCE instead of NO_CONVOY
   - **Fix**: Added check for army moves to non-adjacent without is_convoyed flag
   - **Files**: `diplomacy.c:2688-2693`

5. **test_6_k_2**: (Fixed by convoy handling improvements above)

---

## Previous Session: Convoy & Coast Fixes (Dec 30, 2025 - Session 2)

**Summary**: Fixed convoy path finding for split-coast locations (SPA, BUL, STP) and convoy result codes to distinguish SUCCESS vs NO_CONVOY for unused convoy routes.

### Fixes Implemented

1. **test_6_a_5**: Support validation + test file typo
   - **Issue**: Test file had "F LON" instead of "F NTH" for England's convoy
   - **Fix**: Corrected typo + added validation that support for "move to same location" is invalid
   - **Files**: `test_datc.py:261`, `diplomacy.c:666-670`

2. **test_6_e_11**: Convoy coast adjacency + head-to-head exemption
   - **Issue**: Convoy path finding didn't check coast variants (MAO not adjacent to SPA, only to SPA/SC)
   - **Fix**: Modified `find_convoy_path` to check all coast variants of start/end locations
   - **Issue2**: Head-to-head check voided support during convoy swaps
   - **Fix2**: Exempt convoy swaps from head-to-head restriction
   - **Files**: `diplomacy.c:806-853, 1350-1355`

3. **test_6_a_7**: Convoy order parsing + army validation
   - **Issue**: Convoy order "C LON - BEL" (without "A") was rejected
   - **Fix**: Allow convoy orders without explicit unit type
   - **Issue2**: Convoy for fleet wasn't rejected
   - **Fix2**: Added validation that convoyed unit must be an army
   - **Files**: `diplomacy.c:503-520, 724-743`

4. **test_6_g_6**: Convoy result code for unused routes
   - **Issue**: All convoy orders got SUCCESS even if they weren't on the actual path used
   - **Fix**: Check if removing this fleet breaks the convoy path; if not, fleet gets NO_CONVOY
   - **Files**: `diplomacy.c:2627-2690`

---

## Previous Session: Convoy Paradox Timing Fix (Dec 30, 2025)

**Summary**: Fixed convoy paradox timing issue where convoyed moves could dislodge defenders before paradox detection. The key insight: convoyed moves should NOT set `can_move=1` during initial conflict resolution - this must be deferred to Step 6c after paradox detection. This fixed test 6.F.18 (Betrayal Paradox).

### Fixes Implemented

1. **Defer convoyed move resolution to Step 6c** (diplomacy.c:1489-1528)
   - **Issue**: Convoyed moves set `can_move=1` in Step 4, causing early dislodgements before paradox detection
   - **Fix**: Skip setting `can_move` for convoyed moves in initial conflict resolution
   - **Impact**: Fixed 6.F.18 (Betrayal Paradox)

2. **Step 6c handles convoyed move dislodgements** (diplomacy.c:2188-2300)
   - **Issue**: Step 6c only checked for vacant/vacating destinations, not dislodgements
   - **Fix**: Step 6c now checks if convoyed move can dislodge defender and records dislodgement
   - **Impact**: Allows convoyed moves to dislodge without causing paradox timing issues

---

## Previous Session: Convoy Swap & Civil Disorder Fixes (Dec 29, 2025)

**Summary**: Fixed convoy swaps in head-to-head battles, VIA keyword handling (dislodged unit contesting), and civil disorder BFS distance calculation. Total gain: +9 tests (126->135).

### Fixes Implemented

1. **Convoy swap detection** (diplomacy.c:1474-1508)
   - **Issue**: Convoy swaps (two units passing through each other via convoy) not detected
   - **Fix**: In head-to-head, if either unit is convoyed, check if it's a dislodge or swap
   - **Impact**: Fixed 6.G.9 and related convoy swap tests

2. **VIA keyword dislodged unit contesting** (diplomacy.c:1726-1740)
   - **Issue**: DATC 4.A.7 choice (b) - dislodged unit should still contest if attacker used convoy
   - **Fix**: Only prevent contesting if attacker didn't use convoy (direct dislodgement)
   - **Impact**: Fixed 6.G.10, 6.G.15, 6.G.17, 6.G.18

3. **Civil disorder BFS distance** (diplomacy.c:3036-3145)
   - **Issue**: Civil disorder used last unit in array instead of furthest from home
   - **Fix**: Implemented proper BFS with fleet-only movement for fleets, convoy for armies
   - **Impact**: Fixed 6.J.6, 6.J.8, 6.J.9

4. **Convoy swap dislodgement prevention** (diplomacy.c:1821-1828)
   - **Issue**: Units in convoy swap were being dislodged
   - **Fix**: Skip dislodgement if defender is in head-to-head and either unit is convoyed
   - **Impact**: Prevented false dislodgements in convoy swaps

---

## Previous Session: Adjustment Phase Fixes (Dec 29, 2025)

**Summary**: Fixed adjustment phase (build/disband) result tracking, fleet build validation, and standard mode support. Total gain: +6 tests (119->125).

### Fixes Implemented

1. **Adjustment phase result format** (adapters.py)
   - **Issue**: Results returned wrong format - `[]` instead of `[[]]` for success
   - **Fix**: For adjustment phase, each order result is an element in a list
   - **Impact**: Fixed 6.I.1, 6.I.7

2. **Fleet build validation** (diplomacy.c:2937-2951)
   - **Issue**: Fleets could be built on land-only locations
   - **Fix**: Check LOC_LAND/LOC_PORT for fleet builds, reject if invalid
   - **Impact**: Fixed 6.I.2

3. **Skip coast normalization for BUILD/DISBAND** (binding.c:671)
   - **Issue**: Build orders at coast variants were normalized to existing unit location
   - **Fix**: Skip unit-finding logic for BUILD/DISBAND orders
   - **Impact**: Fixed 6.I.4

4. **Standard mode for DATC tests** (test_datc.py:43-45)
   - **Issue**: DATC tests ran in welfare mode, allowing extra disbands
   - **Fix**: Use `welfare_mode=False` for DATC compliance
   - **Impact**: Fixed 6.J.1

5. **Adjustment result order** (adapters.py:177-182)
   - **Issue**: Multiple orders for same unit stored in wrong order
   - **Fix**: Insert at front of list to match original behavior
   - **Impact**: Fixed 6.I.7

6. **Duplicate check for adjustment phase** (test_datc.py:111-116)
   - **Issue**: Set-based duplicate check failed on `[]` (unhashable)
   - **Fix**: Skip duplicate check for adjustment phase or catch TypeError
   - **Impact**: Enabled adjustment phase result checking

---

## Previous Session: 6.G Convoy Fixes (Dec 29, 2025)

**Summary**: Fixed VOID → NO_CONVOY for unused convoy orders. Total gain: +3 tests (116->119).

### Fixes Implemented

1. **Added RESULT_DISRUPTED code** (diplomacy.h:74)
   - **Issue**: No way to distinguish convoy paradox failure from regular disruption
   - **Fix**: Added RESULT_DISRUPTED = 8 for convoy fleet orders that fail due to Szykman paradox
   - **Impact**: Proper result codes for convoy paradoxes

2. **Disrupted convoys don't contest destination** (diplomacy.c:2049-2053)
   - **Issue**: Failed convoy still marked destination as contested for retreats
   - **Fix**: Skip combat recording if `is_convoyed && convoy_disrupted`
   - **Impact**: Fixed 6.F.7

3. **Disrupted convoys don't compete in strength calculations** (diplomacy.c:1373-1428, 1644-1701)
   - **Issue**: Disrupted convoy moves still blocking other moves to same destination
   - **Fix**: Added convoy disruption detection in Step 4 and Step 5b
   - **Impact**: Fixed 6.F.8

4. **Pandin's Paradox detection** (diplomacy.c:1965-2065)
   - **Issue**: Beleaguered garrison paradox not detected, support incorrectly cut
   - **Fix**: Before cutting support, check if convoy would tip balance and dislodge convoying fleet
   - **Impact**: Fixed 6.F.16, 6.F.17

5. **Support for disrupted convoy gets NO_CONVOY** (diplomacy.c:2453-2473)
   - **Issue**: Support for failed convoyed move showed as SUCCESS
   - **Fix**: Check if supported move was convoyed and disrupted, set RESULT_NO_CONVOY
   - **Impact**: Fixed 6.F.17 (F YOR support result)

6. **convoy_paradox flag for result code selection** (diplomacy.c:955, 2386-2387)
   - **Issue**: Convoy fleet got NO_CONVOY even for paradox disruption
   - **Fix**: Track `convoy_paradox` flag, use DISRUPTED for paradox cases
   - **Impact**: Fixed F ENG result in 6.F.16

---

## Previous Session: Head-to-Head & Build Fixes (Dec 28, 2025)

1. **Fleet build requires coast specification** (diplomacy.c:2598-2605)
   - **Issue**: "F STP B" should be VOID (coast required for fleet builds at split coast locations)
   - **Fix**: Added check in adjustment phase: fleet builds at LOC_PORT require coast
   - **Impact**: Fixed 6.B.14

2. **Units vacating don't block own power's support** (diplomacy.c:1272-1292)
   - **Issue**: Support invalidated when own unit at destination was moving away
   - **Fix**: Check if destination unit is moving to attacker's origin (head-to-head) vs elsewhere
   - **Impact**: Fixed 6.E.9

3. **Dislodged unit doesn't contest destination** (diplomacy.c:1600-1624)
   - **Issue**: Dislodged attacker still competing for destination
   - **Fix**: Skip dislodged attackers when calculating strongest attacker
   - **Impact**: Fixed 6.E.1

4. **Direct swaps (non-convoy) with equal strength bounce** (diplomacy.c:1540-1555)
   - **Issue**: Two-unit cycles allowed as swaps even without convoy
   - **Fix**: In 2-unit cycle without convoy, if neither wins head-to-head, both bounce
   - **Impact**: Fixed 6.E.3

5. **Vacating units not dislodged** (diplomacy.c:1630-1635)
   - **Issue**: Units successfully moving away were being marked as dislodged
   - **Fix**: Skip dislodgement check if defender is successfully moving away (not head-to-head)
   - **Impact**: Fixed false dislodgement in 6.E.9

---

## Previous Session: Split Coast Architecture Fix (Dec 27, 2025)

**Summary**: Fixed fundamental split coast architecture. Parent locations (BUL, SPA, STP) now have full adjacencies, eliminating the need for redundant lowercase variants. Reduced locations from 85 to 82. Total gain: +6 tests (103->109).

### Fixes Implemented

1. **Parent locations have full adjacencies** (diplomacy_map.c)
   - BUL (15), SPA (59), STP (60) now have full land adjacencies
   - Type changed from LOC_COAST to LOC_PORT
   - Armies can now be placed directly on parent locations

2. **Removed redundant lowercase variants** (diplomacy_map.c)
   - Deleted bul (76), spa (79), stp (82)
   - Renumbered coast variants: 76-81 instead of 76-84
   - Reduced total locations from 85 to 82

3. **Fleet adjacency cache respects LOC_PORT** (diplomacy_map.c)
   - Fleets cannot move to/from LOC_PORT directly
   - Must use specific coast variants for fleet movement

4. **Coast inference uses fleet adjacency cache** (diplomacy.c)
   - `default_coast()` now uses adjacency cache to verify reachability
   - Properly handles DATC 6.B.2 (infer coast when only one reachable)

---

## Previous Session: Phase Setting & Convoy Fixes (Dec 22, 2025)

**Summary**: Major progress on multiple fronts. Added `game_set_phase` binding to allow tests to properly set game phase/year. Fixed convoy bounce detection and NO_CONVOY result handling. Implemented anti-kidnapping rule for adjacent convoys. Total gain: +16 tests (87->103).

### Fixes Implemented

1. **`game_set_phase` C binding** (binding.c:533-601)
   - **Issue**: Tests couldn't set game phase - `set_current_phase()` was a no-op
   - **Fix**: Added C function to parse phase strings (e.g., "S1901M", "W1901A") and set game state
   - **Impact**: Unlocked 6.H retreat tests, 6.I build tests, 6.J civil disorder tests
   - **Files**: `binding.c`, `adapters.py`

2. **`last_unit_locations` saved in retreat/adjustment phases** (diplomacy.c:2525-2526, 2635-2636)
   - **Issue**: Order results weren't properly tracked after retreat/adjustment phases
   - **Fix**: Save unit locations to persistent storage in all phase resolution functions
   - **Impact**: Fixed result retrieval for non-movement phases
   - **Files**: `diplomacy.c`

3. **Convoy bounce detection fixed** (diplomacy.c:1834-1871)
   - **Issue**: Convoyed moves only checked against already-resolved moves, not competing moves
   - **Fix**: Check ALL moves targeting same destination, including non-convoyed competing moves
   - **Impact**: +5 tests in 6.F convoy section
   - **Files**: `diplomacy.c`

4. **NO_CONVOY vs BOUNCE result** (diplomacy.c:2036-2038)
   - **Issue**: Disrupted convoys got BOUNCE instead of NO_CONVOY
   - **Fix**: Check `convoy_disrupted` flag before setting BOUNCE
   - **Impact**: Correct result codes for convoy failures
   - **Files**: `diplomacy.c`

5. **Adjacent convoy anti-kidnapping rule** (diplomacy.c:1078-1115)
   - **Issue**: Adjacent moves were forced to use convoy if any power offered it ("kidnapping")
   - **Fix**: Only use convoy for adjacent moves if SAME power offers it (voluntary convoy)
   - **DATC Rule**: 4.A.3 - no forced convoy for adjacent moves
   - **Impact**: Fixed test_6_g_1 (voluntary adjacent convoy)
   - **Files**: `diplomacy.c`

### Results

- **Total gain this session**: +16 tests (87->103, 54.4%->64.4%)
- **Crossed 60% threshold!**
- **Section improvements**:
  - 6.H Retreats: Major improvement (phase setting fix)
  - 6.F Convoys: +5 tests (bounce detection fix)
  - 6.G Adjacent Convoys: +2 tests (anti-kidnapping rule)
  - 6.I Building: Now testable (phase setting fix)
  - 6.J Civil Disorder: Now testable (phase setting fix)

---

## Test Results (136/160 = 85.0%)

| Section | Description | Passed | Total | % | Status |
|---------|-------------|--------|-------|---|--------|
| **6.A** | Basic Validity | 10 | 12 | 83% | Nearly Done |
| **6.B** | Coastal Issues | 14 | 14 | 100% | Done |
| **6.C** | Circular Movement | 5 | 7 | 71% | Good |
| **6.D** | Supports & Dislodges | 30 | 34 | 88% | Good |
| **6.E** | Head-to-Head | 11 | 15 | 73% | Good |
| **6.F** | Convoys | 19 | 24 | 79% | **Improved (+1)** |
| **6.G** | Adjacent Convoys | 13 | 18 | 72% | Good |
| **6.H** | Retreats | 15 | 16 | 94% | Nearly Done |
| **6.I** | Building | 7 | 7 | 100% | Done |
| **6.J** | Civil Disorder | 11 | 11 | 100% | Done |
| **6.K** | Custom | 1 | 2 | 50% | Low Priority |
| **TOTAL** | | **136** | **160** | **85.0%** | **+10 this session** |

---

## Remaining Key Issues (24 failures)

### High Priority

1. **6.F Convoy Paradox** (5 failures)
   - Multi-route convoy paradox (6.F.19-24)
   - 6.F.18 fixed! (Betrayal paradox - timing issue resolved)

2. **6.G Adjacent Convoy** (5 failures)
   - NO_CONVOY result for unused convoys (6.G.6, 6.G.7)
   - Remaining edge cases (6.G.13, 6.G.14, 6.G.16)

3. **6.E Head-to-Head** (4 failures)
   - Support validation edge cases (6.E.9, 6.E.11, 6.E.12, 6.E.15)

### Medium Priority

4. **6.D Support/Dislodge** (4 failures)
   - 6.D.8, 6.D.23, 6.D.31, 6.D.34

5. **6.C Circular Movement** (2 failures)
   - 6.C.2, 6.C.5

6. **6.A Basic Validity** (2 failures)
   - 6.A.5 (VOID + DISLODGED dual result)
   - 6.A.7 (fleet convoy order void)

### Low Priority

7. **6.H Retreat** (1 failure)
   - 6.H.12 (NO_CONVOY vs DISRUPTED result)

8. **6.K Custom** (1 failure)
   - 6.K.2

---

## Visualization System (NEW - Dec 30, 2025)

A Raylib-based visualization system has been added to support:
- **Training visualization**: Watch RL agents play in real-time
- **Debugging/Testing**: Step through phases, inspect orders/results
- **Interactive play**: Human players submit orders via UI

### Files Created

```
pufferlib/ocean/diplomacy/render/
    __init__.py           # Package exports
    map_data.py           # Province coordinates, power colors (200 LOC)
    renderer.py           # Main DiplomacyRenderer class (300 LOC)
    order_viz.py          # Order arrows/lines/curves (200 LOC)
    interactive.py        # Human play controls (200 LOC)
    assets/               # Static assets (map_bg.png - TODO)
```

### Usage

```python
from pufferlib.ocean.diplomacy import Diplomacy

# Create environment with rendering
env = Diplomacy(render_mode="human")
env.reset()

# Render loop
while True:
    env.render()  # Opens Raylib window
    # ... step environment ...
```

### Keyboard Controls

- **WASD/Arrows**: Pan view
- **Q/E or -/+**: Zoom in/out
- **Space**: Reset view
- **Shift**: Turbo mode (3x speed)
- **Tab/~**: Toggle help
- **O**: Toggle orders
- **L**: Toggle labels
- **P**: Pause
- **ESC**: Exit

### Dependencies

- `raylib` and `pyray` (install via `uv pip install raylib pyray`)

---

## Architecture Overview

**Current Stack**:
```
Tests (test_datc.py)
    |
Thin Adapter (adapters.py - 350 LOC)
    |
PufferEnv (diplomacy.py - 200 LOC)
    |
Python Bindings (binding.c - 700 LOC)
    |
Game Engine (diplomacy.c - 2,900 LOC)
Map Data (diplomacy_map.c - 1,273 LOC)
    |
Visualization (render/ - 900 LOC) [NEW]
```

**Key Files**:
- C engine: `pufferlib/ocean/diplomacy/diplomacy.c`
- C map data: `pufferlib/ocean/diplomacy/diplomacy_map.c`
- C header: `pufferlib/ocean/diplomacy/diplomacy.h`
- Bindings: `pufferlib/ocean/diplomacy/binding.c`
- Python env: `pufferlib/ocean/diplomacy/diplomacy.py`
- Visualization: `pufferlib/ocean/diplomacy/render/` (NEW)
- Test adapter: `tests/diplomacy/adapters.py`
- Tests: `tests/diplomacy/original/test_datc.py`

---

## Quick Reference

**Build**:
```bash
python setup.py build_ext --inplace
```

**Run Tests**:
```bash
pytest tests/diplomacy/original/test_datc.py --tb=no -q
pytest tests/diplomacy/original/test_datc.py -k "test_6_f" -v
```

---

## Summary

**Current Status** (Dec 30, 2025):
- **136/160 tests passing (85.0%)** - Crossed 85% threshold!
- **+10 tests this session** (126->136)
- Key fixes: Convoy swap detection, VIA keyword, civil disorder BFS, convoy paradox timing
- Completed sections: 6.B (100%), 6.I (100%), 6.J (100%)
- Major breakthrough: Fixed convoy paradox timing (6.F.18) by deferring convoyed move resolution

**Next Steps**:
1. Fix remaining multi-route convoy paradox (6.F.19-24) - 5 failures
   - 6.F.19: Multi-route - alternate route survives, convoy cuts support
   - 6.F.20: Foreign convoy doesn't count as alternate route
   - These require detecting paradox when alternate convoy routes exist
2. Fix NO_CONVOY result for unused convoys (6.G.6, 6.G.7) - 2 failures
3. Fix support validation edge cases (6.E.9, 6.E.11, 6.E.12) - 3 failures
4. Target: 145+ tests (90%)
