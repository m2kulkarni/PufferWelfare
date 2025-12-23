# Welfare Diplomacy C Port - Implementation Plan

## Overview

Porting welfare-diplomacy from Python to C for PufferLib, achieving 10-100x speedup while maintaining exact functional equivalence.

**Current**: 103/160 DATC tests passing (64.4%) - Updated Dec 22, 2025
**Previous**: 87/160 (54.4%)
**Session Gain**: +16 tests
**Goal**: 120 tests (75%) -> 150+ tests -> 160/160 (100%)

## Architecture

### Core Components
- **C Engine** (~2,900 lines): Game state, order validation, adjudication
- **Python Wrapper** (~200 lines): PufferEnv interface, minimal overhead
- **Adapter Layer** (~350 lines): Compatibility bridge for original tests
- **Original Python Tests** (~6,500 lines): Gold standard for correctness

### Key Simplifications
- No networking, DAIDE protocol, web interface
- No messaging system (can add later for LLM agents)
- Standard map only (hardcoded)
- Fixed 7 players (AUSTRIA, ENGLAND, FRANCE, GERMANY, ITALY, RUSSIA, TURKEY)
- Local simulation only

## Implementation Phases

### Phase 1: Project Structure - COMPLETE
### Phase 2: Core Data Structures - COMPLETE
### Phase 3: Core Game Logic - MOSTLY COMPLETE

#### 3.1 Movement Adjudication - COMPLETE
- Order parsing, validation, basic movement resolution
- Support mechanics (strength calculation, dislodgement, support cutting)
- Result tracking (OK, VOID, BOUNCE, CUT, DISLODGED, NO_CONVOY)

#### 3.2 Convoy Mechanics - IN PROGRESS (58% passing)
- Multi-fleet convoy pathfinding
- Convoy disruption detection
- Basic paradox resolution (Szykman rule)
- Adjacent convoy anti-kidnapping rule (NEW)
- Convoy bounce detection fixed (NEW)
- Complex paradoxes and multi-route scenarios (remaining)

**Current**: 14/24 convoy tests passing (58%)
**Remaining**: ~10 tests (paradox edge cases)

#### 3.3 Split Coasts - NEARLY COMPLETE (93%)
- STP, BUL, SPA coast variants implemented
- Coast-specific adjacencies
- Order parsing with coast specifications
- Parent location comparison for conflicts

**Current**: 13/14 split coast tests passing (93%)
**Remaining**: 1 edge case

#### 3.4 Circular Movement - NEEDS WORK (43%)
- 2-way and 3-way cycles resolved
- Disrupted circular movements handled
- Convoy + circular edge cases remaining

**Current**: 3/7 circular movement tests passing (43%)
**Remaining**: 4 tests (convoy+circular)

### Phase 4: Retreat Phase - NEARLY COMPLETE (94%)
- Calculate valid retreat destinations
- Exclude attacker's origin and contested locations
- Handle multiple retreats to same location (all disband)
- Phase progression for retreat phases
- Convoy detection for adjacent retreats
- Auto-disband for no retreat options
- NO_CONVOY result code
- Coastal crawl prevention
- Contested coast propagation

**Status**: 15/16 retreat tests passing (94%)

### Phase 5: Adjustment Phase - IN PROGRESS (49%)
- Build validation (home center, owned, empty)
- Disband mechanics and civil disorder rules
- Welfare-specific rules (voluntary disbands)
- Phase setting now working (NEW)

**Status**: 9/18 adjustment tests passing (50%)

### Phase 6: RL Interface - PENDING
### Phase 7: Testing & Polish - ONGOING
### Phase 8: Optimization - PENDING

## Current Status & Roadmap

### Current: 103/160 Tests Passing (64.4%)

**Latest Session Progress** (Dec 22, 2025):
- Added `game_set_phase` binding for proper phase/year setting
- Fixed `last_unit_locations` saved in all phases
- Fixed convoy bounce detection against competing moves
- Fixed NO_CONVOY vs BOUNCE result codes
- Implemented adjacent convoy anti-kidnapping rule
- **+16 tests** (87->103)

### Test Coverage by Section

| Section | Description | Passing | % | Priority |
|---------|-------------|---------|---|----------|
| 6.A | Basic Validity | 10/12 | 83% | Low |
| 6.B | Coastal Issues | 13/14 | 93% | Low |
| 6.C | Circular Movement | 3/7 | 43% | Medium |
| 6.D | Supports & Dislodges | 25/34 | 74% | High |
| 6.E | Head-to-Head | 9/15 | 60% | High |
| 6.F | Convoys | 14/24 | 58% | High |
| 6.G | Adjacent Convoys | 5/18 | 28% | High |
| 6.H | Retreats | 15/16 | 94% | Low |
| 6.I | Building | 3/7 | 43% | Medium |
| 6.J | Civil Disorder | 6/11 | 55% | Medium |
| 6.K | Custom | 0/2 | 0% | Low |
| **Total** | | **103/160** | **64%** | |

### Path to 120+ Tests (Next Milestone)

Current: 103 tests -> Goal: 120+ tests (Need +17 tests)

**Priority Fixes**:

1. **Head-to-Head Battles (6.E)** - 6 failures
   - Beleaguered garrison logic
   - Estimated gain: +4-6 tests

2. **Adjacent Convoy Edge Cases (6.G)** - 13 failures
   - Complex disruption scenarios
   - Estimated gain: +5-8 tests

3. **Convoy Paradoxes (6.F)** - 10 failures
   - Szykman rule edge cases
   - Estimated gain: +3-5 tests

4. **Circular Movement (6.C)** - 4 failures
   - Convoy + circular interactions
   - Estimated gain: +2-3 tests

### Path to 150+ Tests (Long-term)

After reaching 120 tests:
- Complete convoy paradox resolution
- Fix all circular movement edge cases
- Complete adjustment phase logic
- Polish edge cases

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

## Recent Progress

### Session: Dec 22, 2025 (+16 tests, 87->103)

**Fixes Completed**:
1. `game_set_phase` C binding - tests can now set phase/year
2. `last_unit_locations` saved in retreat/adjustment phases
3. Convoy bounce detection - check ALL competing moves
4. NO_CONVOY vs BOUNCE result codes
5. Adjacent convoy anti-kidnapping rule (DATC 4.A.3)

**Section Improvements**:
- 6.F Convoys: +5 tests
- 6.G Adjacent Convoys: +2 tests
- 6.H/6.I/6.J: Phase setting now works

## Quick Reference

**Build**:
```bash
python setup.py build_ext --inplace
```

**Run Tests**:
```bash
pytest tests/diplomacy/original/test_datc.py --tb=no -q
pytest tests/diplomacy/original/test_datc.py -k "test_6_e" -v
```

**Key Files**:
- C engine: `pufferlib/ocean/diplomacy/diplomacy.c` (2,900 LOC)
- C bindings: `pufferlib/ocean/diplomacy/binding.c` (700 LOC)
- Test adapter: `tests/diplomacy/adapters.py` (350 LOC)

## Summary

**Current**: 103/160 tests (64.4%) - crossed 60% threshold!
**Target**: 120 tests (75%)
**Gap**: +17 tests needed
**Next Action**: Fix head-to-head battles (6.E) and remaining convoy issues (6.F/6.G)

**Recent Achievement**: +16 tests in single session! Phase setting, convoy bounce detection, and anti-kidnapping rule all fixed.
