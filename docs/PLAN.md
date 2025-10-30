# Welfare Diplomacy C Port - Implementation Plan

## Overview

Porting welfare-diplomacy from Python to C for PufferLib, achieving 10-100x speedup while maintaining exact functional equivalence.

**Current**: 92/160 DATC tests passing (57.5%) - Updated Oct 30, 2025 🎉
**6.B+6.D**: 36/48 tests passing (75%) - Major breakthrough!
**Goal**: 100 tests (62.5%) → 150+ tests → 160/160 (100%)

## Architecture

### Core Components
- **C Engine** (~2,500 lines): Game state, order validation, adjudication
- **Python Wrapper** (~200 lines): PufferEnv interface, minimal overhead
- **Adapter Layer** (~650 lines): Compatibility bridge for original tests
- **Original Python Tests** (~6,500 lines): Gold standard for correctness

### Key Simplifications
- No networking, DAIDE protocol, web interface
- No messaging system (can add later for LLM agents)
- Standard map only (hardcoded)
- Fixed 7 players (AUSTRIA, ENGLAND, FRANCE, GERMANY, ITALY, RUSSIA, TURKEY)
- Local simulation only

## Implementation Phases

### Phase 1: Project Structure ✅ COMPLETE
**Deliverables:**
- Directory structure: `pufferlib/ocean/diplomacy/`
- Files: `binding.c`, `diplomacy.h`, `diplomacy.c`, `diplomacy.py`
- Build system integrated into `setup.py`
- C extension imports successfully

### Phase 2: Core Data Structures ✅ COMPLETE
**Deliverables:**
- Map structure (76 locations, adjacency cache)
- Power structure (units, centers, orders, welfare tracking)
- Game state structure (phase progression, combat resolution)
- PufferLib environment structure

### Phase 3: Core Game Logic ✅ MOSTLY COMPLETE

#### 3.1 Movement Adjudication ✅ COMPLETE
- Order parsing, validation, basic movement resolution
- Support mechanics (strength calculation, dislodgement, support cutting)
- Result tracking (OK, VOID, BOUNCE, CUT, DISLODGED)

#### 3.2 Convoy Mechanics ⏳ IN PROGRESS
- Multi-fleet convoy pathfinding ✅
- Convoy disruption detection ✅
- Basic paradox resolution (Szykman rule) ✅
- Adjacent convoy moves (recent fix) ✅
- Complex paradoxes and multi-route scenarios ⏳

**Current**: 8/24 convoy tests passing (33%)
**Remaining**: ~16 tests (convoy pathfinding edge cases)

#### 3.3 Split Coasts ⏳ NEEDS WORK
- STP, BUL, SPA coast variants implemented ✅
- Coast-specific adjacencies ✅
- Order parsing with coast specifications ⏳
- Coast inference and validation ⏳

**Current**: 4/14 split coast tests passing (28%)
**Remaining**: ~10 tests (coast specification handling)

#### 3.4 Circular Movement ✅ MOSTLY WORKING
- 2-way and 3-way cycles resolved ✅
- Disrupted circular movements handled ✅
- Support orders in circular movements ✅

**Current**: 6/7 circular movement tests passing (86%)
**Remaining**: 1 convoy+circular edge case

### Phase 4: Retreat Phase ✅ NEARLY COMPLETE
**Deliverables:**
- ✅ Calculate valid retreat destinations
- ✅ Exclude attacker's origin and contested locations (combat tracking fixed Oct 29)
- ✅ Handle multiple retreats to same location (all disband)
- ✅ Update phase progression for retreat phases
- ✅ Convoy detection for adjacent retreats (fixed Oct 29)
- ✅ Auto-disband for no retreat options
- ✅ NO_CONVOY result code for disrupted convoy orders
- ✅ Support order validation (check actual unit orders)
- ✅ Coastal crawl prevention (DATC 6.H.15)
- ✅ Contested coast propagation (DATC 6.H.16)
- ⏸️ test_6_h_12 edge case (adjacent convoy disruption)

**Status**: 15/16 retreat tests passing (93.75%) - Recent gain: +14 tests (1→15)
**Remaining**: test_6_h_12 (complex adjacent convoy edge case - appears to be adapter state issue)

### Phase 5: Adjustment Phase 🎯 CRITICAL PRIORITY
**Deliverables:**
- Build validation (home center, owned, empty)
- Disband mechanics and civil disorder rules
- Welfare-specific rules (voluntary disbands)
- Welfare point calculation: `max(0, num_centers - num_units)`

**Impact**: +18 direct tests + unblocks remaining split coast tests = +25 tests
**Target**: ~99 → ~124 tests (77%)

### Phase 6: RL Interface
**Deliverables:**
- Observation space (board, units, phase, year, centers, welfare, valid actions)
- Action space (discrete, ~1000 orders)
- Action masking for invalid orders
- Reward calculation (welfare deltas)
- Multi-agent support (7 powers)

### Phase 7: Testing & Polish
**Deliverables:**
- 100% DATC pass rate (all 159 tests)
- All original Python tests passing
- Edge case handling
- Memory leak checks
- Performance benchmarks

### Phase 8: Optimization
**Deliverables:**
- Memory layout optimization
- Lookup table pre-computation
- SIMD vectorization (if beneficial)
- Multi-threading for vectorized envs

## Current Status & Roadmap

### Current: 86/160 Tests Passing (53.75%) 🎉 PROGRESSING TO 100!
**Recent Progress**: +5 tests in latest session (81→86)
- Basic movement, support mechanics working ✅
- Retreat phase nearly complete (15/16, 93.75%) 🎉
- Dislodgement-cuts-support logic fixed (6.D.17) ✅
- Coast normalization in support matching implemented ⏳
- Coast variant checking in validation added ⏳
- Convoy basics working (8/24) ⏳
- Split coasts improving (6/14, 43%) ⏳

### Path to 100 Tests (Near-term Goal) - Updated!

Current: 86 tests → Goal: 100+ tests (Need +14 tests)

See detailed roadmap below for revised phases:
1. ~~**Complete Retreat Phase**~~ ✅ DONE → 81 tests
2. ~~**Start 6.D.17 Fix**~~ ✅ DONE → 86 tests (+5)
3. **Fix Remaining 6.D Support Validation** → 96 tests (+10, complex)
4. **Fix Remaining 6.B Coastal Handling** → 104 tests (+8, complex)
5. **Quick Wins in Other Sections** → 110+ tests (+6+)

### Path to 150+ Tests (Long-term)

After reaching 100 tests, focus shifts to:
- **Beleaguered Garrison Logic** (6.E section)
- **Complex Convoy Paradoxes** (remaining 6.F/6.G)
- **Circular Movement Edge Cases** (6.C section)
- **Adjustment Phase Edge Cases** (6.I/6.J sections)

## Testing Strategy

### Test Hierarchy

#### Level 1: C-Specific Tests
**Location**: `tests/diplomacy/c_tests/`
- `test_c_binding.py` - C extension loading, memory management
- `test_c_map.py` - Map data structures
- `test_c_game_state.py` - Game state management
- `test_c_orders.py` - Order parsing and validation
- `test_c_welfare.py` - Welfare calculations
- `test_simple_adjudication.py` - Simple movement scenarios

**Purpose**: Test C implementation in isolation

#### Level 2: Original Python Tests (Gold Standard)
**Location**: `tests/diplomacy/original/`
- `test_map.py` - Map functionality (238 lines)
- `test_game.py` - Game logic (669 lines)
- `test_datc.py` - DATC compliance (5478 lines, ~159 tests)

**Purpose**: Prove exact functional equivalence with Python

#### Level 3: Adapter Layer
**Location**: `tests/diplomacy/adapters.py`
- `GameAdapter` - Bridges C implementation to Python Game API
- `MapAdapter` - Bridges C map to Python Map API

**Purpose**: Make C implementation compatible with original tests

### DATC Test Coverage by Section (Current: 86/160 = 53.75%)

| Section | Description | Passing | Priority | Impact |
|---------|-------------|---------|----------|--------|
| 6.A | Basic Validity | 11/12 (92%) | 🟢 Nearly Done | +1 test (edge case) |
| 6.B | Coastal Issues | 12/14 (86%) | 🎉 Nearly Done | +2 tests (edge cases) |
| 6.C | Circular Movement | 6/7 (85%) | 🟢 Strong | +1 test (convoy+circular) |
| 6.D | Supports & Dislodges | 24/34 (71%) | 🟠 High | +10 tests (validation) |
| 6.E | Head-to-Head | 6/15 (40%) | 🟡 Medium | +9 tests (beleaguered) |
| 6.F | Convoys | 8/24 (33%) | 🟠 High | +16 tests (pathfinding) |
| 6.G | Adjacent Convoys | 4/18 (22%) | 🟠 High | +14 tests (adjacent edge) |
| 6.H | Retreats | 15/16 (93%) | 🎉 Nearly Done | +1 test (edge case) |
| 6.I | Building | 5/7 (71%) | 🟢 Low | +2 tests (adjustment) |
| 6.J | Civil Disorder | 11/11 (100%) | 🎉 COMPLETE | +0 tests |
| 6.K | Custom | 0/2 (0%) | 🟡 Low | +2 tests (edge cases) |
| **Total** | | **92/160 (57.5%)** | **Target: 100+** | **+8 needed** |

### Test Execution Strategy

**Fast Feedback Loop**:
```bash
# C-specific tests only (fast)
pytest tests/diplomacy/c_tests/ -v

# Specific DATC section
pytest tests/diplomacy/original/test_datc.py -k "test_6_d" -v

# All passing tests
pytest tests/diplomacy/original/test_datc.py -v
```

**Full Validation**:
```bash
# All tests (slow)
pytest tests/diplomacy/ -v

# DATC suite only
pytest tests/diplomacy/original/test_datc.py -v
```

## Technical Decisions

### Language Choice
- **C** (not C++) for simplicity and PufferLib consistency
- C99 standard features
- Clean, readable code prioritized over performance (initially)

### Map Data
- Compile-time data structures (no runtime parsing)
- Standard Diplomacy map hardcoded as static arrays
- 76 locations, 34 supply centers, 7 powers

### Order Representation
- String format for human readability during development
- Integer encoding deferred until optimization phase

### State Hashing
- Skip Zobrist hashing initially
- Add later if needed for transposition tables

### Vectorization
- Follow PufferLib `vec_init`/`vec_step` pattern
- Support 100+ parallel games in single process

## Roadmap: 81 → 100+ Tests (Updated Oct 29, 2025)

### ~~Phase 1: Complete Retreat Phase~~ ✅ COMPLETED
**Target**: 70 → 81 tests (+11) - **EXCEEDED!**
**Result**: 81 tests (50.625%)
**Status**: DONE - 15/16 tests passing (93.75%)

**Completed Fixes**:
1. ✅ Auto-disband for no retreat options (test_6_h_9)
2. ✅ NO_CONVOY result code for disrupted convoys (test_6_h_13, test_6_h_14)
3. ✅ Support order validation - check actual unit orders (+6 tests across sections)
4. ✅ Coastal crawl prevention (test_6_h_15 - DATC 6.H.15)
5. ✅ Contested coast propagation (test_6_h_16 - DATC 6.H.16)

**Impact**: +11 tests total (70→81), far exceeding the +6 target!

**Remaining**: test_6_h_12 (complex adjacent convoy edge case - deferred as low priority)

---

### Phase 2: Fix Dislodgement After Failed Move 🔴 NEXT - CRITICAL
**Target**: 81 → 89 tests (+8)
**Effort**: Medium (3-4 hours)
**Priority**: Critical - blocks many 6.A and 6.D tests

**Root Cause**: Units that BOUNCE are not being dislodged when origin is captured

**Example**:
```
A WAR -> MOS (bounces)
A GAL -> WAR (succeeds)
Expected: WAR unit = BOUNCE + DISLODGED
Actual: WAR unit = BOUNCE only ❌
```

**Fix Location**: `diplomacy.c:apply_successful_moves()` step 6a (incomplete)

**Tests Affected**: test_6_a_5, test_6_d_7, test_6_d_9, ~5 more

---

### Phase 3: Fix Coastal Specification 🟠 HIGH IMPACT
**Target**: 89 → 97 tests (+8)
**Effort**: Medium (3-5 hours)
**Priority**: High - relatively straightforward fixes

**Issues**:
1. Coast inference (if only one coast adjacent, use it)
2. Ambiguous orders rejection (if multiple coasts possible)
3. Coast validation in support orders

**Locations**: SPA (NC/SC), BUL (EC/SC), STP (NC/SC)

**Fix Locations**:
- `diplomacy.c:validate_move_order()`
- `diplomacy.c:validate_support_order()`

---

### Phase 4: Improve Convoy Pathfinding 🟠 HIGH IMPACT
**Target**: 97 → 109 tests (+12)
**Effort**: High (5-8 hours)
**Priority**: High - biggest potential gain

**Sub-phases**:
1. **Convoy disruption** (+4 tests) - attacked/dislodged fleets break chain
2. **Adjacent convoy handling** (+4 tests) - convoyed moves to adjacent locations
3. **Multi-route convoys** (+4 tests) - multiple path selection

**Fix Locations**:
- `diplomacy.c:find_convoy_path()` - pathfinding logic
- `diplomacy.c:resolve_movement_phase()` - disruption handling

---

### Phase 5: Polish & Edge Cases 🟡 CLEANUP
**Target**: 109 → 115+ tests (+6+)
**Effort**: Medium (2-4 hours)
**Priority**: Medium - mop up remaining issues

**Areas**:
- Circular movement edge cases (6.C: +4 tests)
- Beleaguered garrison logic (6.E: +9 tests, may need deeper work)
- Civil disorder edge cases (6.J: +6 tests)
- Custom tests (6.K: +2 tests)

## Success Criteria

1. **Correctness**: 100% DATC test pass rate (159/159)
2. **Equivalence**: All original Python tests pass
   - test_map.py ✓
   - test_game.py ✓
   - test_datc.py ✓
3. **Performance**: 10-100x faster than Python
4. **Stability**: No memory leaks, no crashes
5. **Usability**: Clean Python API, easy PufferLib integration

## Recent Progress (Oct 29, 2025)

### Latest Session: +11 Tests (70 → 81) 🎉 CROSSED 50%!

**Fixes Completed**:
1. ✅ **NO_CONVOY result code** (diplomacy.h, diplomacy.c:1912-1947, adapters.py)
   - Added RESULT_NO_CONVOY = 7 enum for disrupted convoy orders
   - Convoy validation checks if convoy was used and if it was disrupted
   - Impact: +2 tests directly (test_6_h_13, test_6_h_14)

2. ✅ **Support order validation** (diplomacy.c:1966-1999)
   - Support orders now check if supported unit is actually making the supported move
   - Mark VOID if unit isn't making the move being supported
   - Impact: +6 tests across multiple sections (6.C, 6.D, 6.H)

3. ✅ **Coastal crawl prevention** (diplomacy.c:2087-2113)
   - Units cannot retreat to other coast of attacker's origin (DATC 6.H.15)
   - Base territory name comparison to detect same-territory different-coast
   - Impact: +1 test (test_6_h_15)

4. ✅ **Contested coast propagation** (diplomacy.c:2051-2085)
   - If one coast is contested, all coasts of that territory are unavailable (DATC 6.H.16)
   - Combat tracking checks for same base territory across coast variants
   - Impact: +1 test (test_6_h_16)

**Section Improvements**:
- 6.H Retreats: 11/16 → 15/16 (68% → 93%) ⬆ +4 tests
- 6.C Circular Movement: 3/7 → 6/7 (42% → 85%) ⬆ +3 tests
- 6.D Supports & Dislodges: 15/34 → 18/34 (44% → 52%) ⬆ +3 tests
- **Overall**: 70/160 → 81/160 (43.75% → 50.625%) ⬆ +11 tests

**Key Achievement**: Crossed the 50% threshold! Nearly completed retreat phase (93.75%).

---

### Previous Session: +6 Tests (64 → 70)

**Fixes Completed**:
1. ✅ **Unit type mapping bug** (adapters.py) - Fixed enum mismatch (`type==1` is ARMY not Fleet)
   - Impact: +5 tests, fixed dislodgement detection across sections

2. ✅ **Combat tracking** (diplomacy.c:1827-1855) - Added `game->combats[]` population
   - Impact: +1 test (test_6_h_6), contest location exclusion for retreats

3. ✅ **Convoy detection** (diplomacy.c:1023-1030) - Fixed adjacent convoyed moves
   - Impact: Fixed test_6_h_11 (retreat via convoy to adjacent location)

**Section Improvements**:
- 6.H Retreats: 1/16 → 10/16 (6% → 62%) ⬆ +9 tests

---

## Quick Action Plan (Updated)

**Fastest path to 100+ tests from 81:**

1. ~~**Complete Retreat Phase**~~ ✅ DONE → 81 tests
   - ✅ Fixed NO_CONVOY result code
   - ✅ Fixed support order validation
   - ✅ Fixed coastal retreat logic

2. **Fix Dislodgement Logic** → 89 tests (3-4 hours)
   - Complete step 6a in apply_successful_moves()
   - Check bounced units for origin capture
   - **NEXT PHASE**

3. **Fix Coastal Handling** → 97 tests (3-5 hours)
   - Coast inference and validation
   - Support order coast checks

4. **Improve Convoy Logic** → 109 tests (5-8 hours)
   - Convoy disruption handling
   - Multi-route pathfinding

5. **Polish Edge Cases** → 115+ tests (2-4 hours)
   - Beleaguered garrison logic
   - Civil disorder, custom tests

**Estimated time to 100 tests**: 11-17 hours (down from 15-25!)
**Estimated time to 110 tests**: 13-21 hours (down from 17-29!)

## Next Immediate Steps 🎯

### ~~Step 1: Complete Retreat Phase~~ ✅ DONE

**Result**: 81 tests (50.625%), 15/16 on section 6.H (93.75%)

**Completed**:
- ✅ NO_CONVOY result code for disrupted convoys
- ✅ Support order validation
- ✅ Coastal crawl prevention
- ✅ Contested coast propagation

**Deferred**: test_6_h_12 (complex adjacent convoy edge case - low priority)

---

### Step 2: Fix Dislodgement Logic (NEXT - Phase 2)

Investigate failing tests:
```bash
pytest tests/diplomacy/original/test_datc.py::TestDATC::test_6_a_5 -vv
pytest tests/diplomacy/original/test_datc.py::TestDATC::test_6_d_7 -vv
```

Fix location: `diplomacy.c:apply_successful_moves()` step 6a
- Check if bounced units were attacked at origin
- Mark as dislodged if origin was captured

Expected gain: +8 tests → 89 total (55%)

---

## Notes

- This plan evolves as implementation progresses
- Correctness comes before performance
- Testing is not optional - exact equivalence required
- All deviations from Python behavior must be documented
- Original Python tests at `../welfare-diplomacy` serve as gold standard

---

## Summary

**Current**: 92/160 tests (57.5%) 🎉 APPROACHING 100 TESTS!
**Target**: 100 tests (62.5%)
**Gap**: +8 tests needed
**Estimated Effort**: 4-8 hours for final push to 100
**Next Action**: Fix remaining 6.D support validation (10 tests) - impossible move handling

**Recent Achievement**: Fixed coastal bounce detection with parent location comparison! Section 6.B improved from 7/14 (50%) to 12/14 (86%). Section 6.J completed (11/11, 100%). Total gain: +6 tests in this session (86→92).
