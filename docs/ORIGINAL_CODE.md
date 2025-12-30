# Original Welfare-Diplomacy Code Reference

**Purpose**: This document provides a comprehensive explanation of how the original Diplomacy adjudication works, based on Philip Paquette's diplomacy library and the DATC (Diplomacy Adjudication Test Cases) specification. This serves as the authoritative reference for our C port.

## Table of Contents
1. [Core Concepts](#core-concepts)
2. [Order Types](#order-types)
3. [Phase Resolution](#phase-resolution)
4. [Movement Phase Algorithm](#movement-phase-algorithm)
5. [Head-to-Head Battles](#head-to-head-battles)
6. [Beleaguered Garrison Rule](#beleaguered-garrison-rule)
7. [Convoy Mechanics](#convoy-mechanics)
8. [Convoy Paradoxes (Szykman Rule)](#convoy-paradoxes-szykman-rule)
9. [Adjacent Convoy Rules](#adjacent-convoy-rules)
10. [Circular Movement](#circular-movement)
11. [Support Mechanics](#support-mechanics)
12. [Retreat Phase](#retreat-phase)
13. [Adjustment Phase](#adjustment-phase)
14. [Result Codes](#result-codes)

---

## Core Concepts

### Map Structure
- **76 base locations** + **6 split coast variants** = **82 total locations**
- Split coast locations: STP (North/South), BUL (East/South), SPA (North/South)
- Location types: LAND, COAST, WATER, PORT (split coast parents)

### Powers
7 powers: AUSTRIA, ENGLAND, FRANCE, GERMANY, ITALY, RUSSIA, TURKEY

### Units
- **Army (A)**: Can move on LAND and COAST, cannot convoy
- **Fleet (F)**: Can move on WATER and COAST, can convoy armies, must specify coast for split coast locations

### Game Phases
```
SPRING MOVEMENT → SPRING RETREAT → FALL MOVEMENT → FALL RETREAT → WINTER ADJUSTMENT → (next year)
```

---

## Order Types

### Movement Phase Orders
| Order | Syntax | Description |
|-------|--------|-------------|
| HOLD | `A PAR H` | Unit holds position |
| MOVE | `A PAR - BUR` | Unit moves to adjacent location |
| SUPPORT HOLD | `A PAR S A MAR` | Support unit to hold |
| SUPPORT MOVE | `A PAR S A MAR - BUR` | Support unit to move |
| CONVOY | `F ENG C A LON - BRE` | Fleet convoys army |

### Retreat Phase Orders
| Order | Syntax | Description |
|-------|--------|-------------|
| RETREAT | `A PAR R BUR` | Dislodged unit retreats |
| DISBAND | `A PAR D` | Dislodged unit disbands |

### Adjustment Phase Orders
| Order | Syntax | Description |
|-------|--------|-------------|
| BUILD | `A PAR B` | Build new unit at home center |
| DISBAND | `A PAR D` | Remove unit (when over SC count) |

---

## Phase Resolution

### Phase Flow
```
1. All powers submit orders simultaneously
2. Orders are validated (VOID if invalid)
3. Conflicts are resolved according to rules
4. Results are applied atomically
5. Game advances to next phase
```

### Key Principle: Simultaneous Resolution
All orders are considered to happen at the same instant. There is no "order of operations" - all moves either succeed or fail based on the final state of all interactions.

---

## Movement Phase Algorithm

The movement phase is the most complex. Here's the canonical algorithm:

### Step 1: Order Collection
```
FOR each power:
    FOR each unit without explicit order:
        Create implicit HOLD order
    Collect all MOVE, SUPPORT, and CONVOY orders
```

### Step 2: Support Cut Detection
```
FOR each MOVE order (non-convoyed):
    IF move targets a supporter's location:
        IF attacker is NOT the unit being supported:
            IF attacker origin != support destination (DATC 6.D.15):
                Mark support as CUT
```

**Critical Exception**: A move to a location does NOT cut support if:
- The moving unit is the one being supported
- The moving unit comes FROM the destination of the supported move (defender can't cut support for attack on itself)

### Step 3: Hold Support Invalidation
```
FOR each HOLD support:
    IF supported unit has a VALID move order (to_location >= 0):
        Invalidate the hold support (DATC 6.D.7)
```

**Exception**: If the supported unit's move is VOID (invalid), it's treated as a hold and CAN receive hold support.

### Step 4: Strength Calculation
```
FOR each MOVE order:
    attack_strength = 1  # base strength
    FOR each valid, uncut SUPPORT for this move:
        IF supporter's power != target unit's power:
            attack_strength += 1
        ELSE:
            # Cannot support dislodging own unit (DATC 6.E.2)
            Invalidate support

FOR each unit (defender):
    defend_strength = 1  # base strength
    FOR each valid, uncut HOLD support for this unit:
        defend_strength += 1
```

### Step 5: Conflict Resolution
```
FOR each destination being attacked:
    Find all attackers to this destination
    Find defender at destination (if any)

    IF multiple attackers with equal MAX strength:
        ALL attackers BOUNCE
    ELSE IF single strongest attacker:
        IF attacker_strength > defender_strength:
            Attacker succeeds, defender dislodged
        ELSE:
            Attacker bounces
```

### Step 6: Circular Movement Detection
```
FOR each unresolved move:
    Follow chain: unit → destination → unit at dest → ...
    IF chain returns to starting unit:
        This is a circular movement
        Check if any external attacker blocks any unit in cycle
        IF no external blockers AND not same-power swap:
            All units in cycle move successfully
```

### Step 7: Apply Moves
```
FOR each successful move:
    Move unit to destination
FOR each dislodged unit:
    Remove from board (to retreat phase)
```

---

## Head-to-Head Battles

**Definition**: Two units moving to each other's locations.

### Resolution Algorithm
```
A VIE (str 2) → TRI
F TRI (str 1) → VIE

IF A.attack_strength > F.attack_strength:
    A succeeds, F dislodged
ELSE IF F.attack_strength > A.attack_strength:
    F succeeds, A dislodged
ELSE:
    BOTH bounce (equal strength)
```

### Critical Rules

#### DATC 6.E.1: Dislodged Unit Has No Effect
A unit that is dislodged in a head-to-head battle does NOT contest the destination for other attackers.

```
Example:
A VIE - TRI (strength 3)
F TRI - VIE (strength 1)
A RUH - TRI (strength 2)

Result: A VIE wins head-to-head, F TRI dislodged.
        A VIE moves to TRI.
        A RUH does NOT bounce with F TRI (F TRI is dislodged).
```

#### DATC 6.E.3: Direct Swaps Require Convoy
Two units cannot swap positions via direct (non-convoyed) moves. Both bounce.

```
Example:
A LON - YOR
A YOR - LON

Result: BOTH bounce (cannot swap without convoy)
```

**Exception**: If one or both units use convoy, swapping IS allowed.

#### DATC 6.E.9: Vacating Unit Doesn't Block Support
If a unit is moving AWAY (not head-to-head), it doesn't block support for the attacker.

```
Example:
F NTH - NWG (moving away)
F NWY - NTH (attacking NTH)
F YOR S F NWY - NTH (support valid!)

Result: F NTH successfully vacates, F NWY moves in.
        The support from YOR is valid because NTH is not staying to fight.
```

---

## Beleaguered Garrison Rule

**Definition**: A unit is attacked from multiple directions with equal strength, so it's NOT dislodged.

### Algorithm
```
FOR each unit being attacked:
    Find all attackers with their strengths
    IF multiple attackers with equal MAX strength:
        Unit is NOT dislodged (beleaguered garrison)
        ALL attackers bounce
```

### Critical Rule: Non-Dislodged Loser Still Has Effect

**DATC 6.E.4-6**: Even if a unit is NOT dislodged (due to beleaguered garrison), it still "blocks" its location for purposes of other moves.

```
Example (6.E.4):
Germany: F HOL - NTH (str 3)
France:  F NTH - HOL (str 2)
England: F NWG - NTH (str 3)
Austria: A RUH - HOL (str 2)

Analysis:
- F HOL and F NWG both attack F NTH with strength 3
- F NTH is NOT dislodged (beleaguered garrison)
- F NTH was trying to move to HOL
- Even though F NTH doesn't succeed in moving, its ATTEMPT to move
  still contests HOL
- Therefore A RUH cannot move to HOL (bounces against F NTH)

Result: ALL units stay in place
```

### Self-Dislodgement with Beleaguered Garrison

**DATC 6.E.7-8**: A power cannot dislodge its own unit, even with help from an ally.

```
Example (6.E.7):
England: F NTH H, F YOR S F NWY - NTH
Germany: F HOL S F HEL - NTH, F HEL - NTH
Russia:  F SKA S F NWY - NTH, F NWY - NTH

Analysis:
- Russia attacks NTH with strength 3 (2 support + own)
- Germany attacks NTH with strength 2
- F YOR (England) supports Russia's attack
- BUT F NTH is also English
- Cannot count English support for attack on English unit
- Without English support, Russia has strength 2 = Germany's 2
- F NTH is beleaguered garrison, NOT dislodged

Result: F NTH holds, all attackers bounce
```

---

## Convoy Mechanics

### Convoy Path
An army can be convoyed if there exists a path of fleets in WATER spaces connecting the army's origin to its destination.

```
A LON - BRE (needs convoy)
F ENG C A LON - BRE

Path: LON → ENG (fleet) → BRE ✓
```

### Multi-Fleet Convoy
```
A LON - NAP
F ENG C A LON - NAP
F MAO C A LON - NAP
F WES C A LON - NAP

Path: LON → ENG → MAO → WES → NAP ✓
```

### Convoy Disruption
A convoy is disrupted if ALL paths are broken (all convoying fleets dislodged).

**DATC 6.F.9-10**: If multiple routes exist and only some are disrupted, convoy still succeeds via remaining routes.

```
Example:
A LON - BEL
F ENG C A LON - BEL (dislodged)
F NTH C A LON - BEL (NOT dislodged)

Result: Convoy succeeds via NTH
```

### Disrupted Convoy Effects

**DATC 6.F.6**: Dislodged convoy does NOT cut support.
```
A LON - HOL via convoy (convoy disrupted)
A HOL S A BEL

Result: A HOL's support is NOT cut (convoy failed before landing)
```

**DATC 6.F.7-8**: Disrupted convoy does NOT contest destination.
```
A LON - HOL via convoy (convoy disrupted)
A BEL - HOL

Result: A BEL moves to HOL (no contest from disrupted convoy)
```

---

## Convoy Paradoxes (Szykman Rule)

### What is a Paradox?
A paradox occurs when the outcome of a convoy depends on whether the convoy succeeds, creating circular logic.

### The Classic Paradox (6.F.14)
```
England: F LON S F WAL - ENG
England: F WAL - ENG
France:  A BRE - LON (via convoy)
France:  F ENG C A BRE - LON

Paradox:
- IF convoy succeeds: A BRE attacks LON, cuts support, F ENG survives
- IF convoy fails: Support not cut, F WAL dislodges F ENG, convoy fails
- Self-referential loop!
```

### The Szykman Rule (Our Implementation)
"If a convoy is part of a paradox, the convoy FAILS and does NOT cut support."

```
Resolution of 6.F.14:
1. Detect paradox: A BRE's success depends on F ENG surviving
2. Apply Szykman: Convoy fails
3. Result: Support not cut, F WAL dislodges F ENG
```

### Paradox Detection Algorithm
```
FUNCTION detect_paradox(army_dest, fleet_location):
    # Check if destination unit supports the convoying fleet
    FOR order at army_dest:
        IF order is SUPPORT for fleet_location:
            RETURN TRUE  # Paradox detected
    RETURN FALSE
```

### Key Paradox Test Cases

| Test | Scenario | Expected Result |
|------|----------|-----------------|
| 6.F.14 | Simple paradox | Fleet dislodged, convoy fails |
| 6.F.16 | Pandin's paradox (beleaguered) | Fleet NOT dislodged, convoy fails |
| 6.F.17 | Extended Pandin's | Fleet NOT dislodged, convoy fails |
| 6.F.18 | Betrayal paradox | Convoy fails, fleet survives |

### DISRUPTED Result Code
When a convoy is part of a paradox and fails under Szykman rule, use DISRUPTED (not NO_CONVOY) to indicate the fleet's order was valid but disrupted by paradox resolution.

---

## Adjacent Convoy Rules

### The Kidnapping Problem (DATC 4.A.3)
When an army can reach its destination either by land OR by convoy, which route does it take?

### Our Rule (1982/2000 Rulebook)
**Voluntary Convoy**: Army uses convoy ONLY if its own power ordered the convoy.

```
Example:
England: A LON - BEL
France:  F ENG C A LON - BEL  # Enemy offering convoy

Result: A LON moves by LAND (ignores enemy's convoy offer)
```

### Same-Power Convoy = Voluntary
```
Example:
England: A LON - BEL
England: F ENG C A LON - BEL  # Own fleet offering convoy

Result: A LON moves by CONVOY (player's choice)
```

### Algorithm
```
FOR each army move:
    IF destination is land-adjacent:
        IF own power ordered convoy:
            Use convoy (if path exists)
        ELSE:
            Use land route (ignore enemy convoys)
    ELSE:
        Must use convoy (no land route)
```

### Result Codes for Unused Convoys
When a fleet offers to convoy but the convoy is not used:
- **NO_CONVOY**: Fleet's convoy order was valid but unused (army took land route or convoy failed)
- **VOID**: Fleet's convoy order was invalid (wrong locations, coastal fleet, etc.)

---

## Circular Movement

### Definition
Units moving in a cycle: A→B, B→C, C→A

### Resolution
Circular movements succeed IF:
1. All units in cycle have valid move orders
2. No external attacker blocks any unit in the cycle
3. Not a same-power 2-unit swap (without convoy)

### Algorithm
```
FOR each unresolved move:
    Follow destination chain
    IF returns to origin:
        cycle_units = units in cycle
        FOR each unit in cycle:
            FOR each external attacker to unit's destination:
                IF attacker_strength >= cycle_unit_strength:
                    Cycle FAILS, all bounce
        IF no external blocker:
            All units in cycle move successfully
```

### Special Case: 2-Unit Cycle Without Convoy (DATC 6.E.3)
```
A LON - YOR
A YOR - LON

Result: BOTH bounce (direct swap not allowed without convoy)
```

### External Attacker Blocks Cycle
```
A PAR - BUR
A BUR - MAR
A MAR - PAR
A MUN - BUR (external attacker, strength 1)

Result: If A MUN's strength >= A PAR's strength, cycle fails
```

---

## Support Mechanics

### Support Validity
Support is VALID if:
1. Supporting unit CAN reach the destination (for its unit type)
2. Supported unit exists at specified location
3. Support destination is adjacent to supporter

### Support Cutting
Support is CUT if:
1. Supporter is attacked by a unit NOT being supported
2. Attacker is NOT coming from support destination (DATC 6.D.15)
3. Attack is not via disrupted convoy

### Self-Support
A unit CANNOT support itself (DATC 6.A.8).
```
F TRI S F TRI  # VOID - self-support not allowed
```

### Support for Own Dislodgement (DATC 6.E.2)
A power CANNOT support an attack that would dislodge their own unit.
```
Italy:   A VIE - TRI (with French support)
Austria: F TRI H
France:  A MUN S A VIE - TRI

If Austria and Italy are same power:
Result: French support is VOID (would dislodge own Austrian fleet)
```

### Support to Unreachable Coast Allowed (DATC 6.B.4)
```
F MAR S F GAS - SPA/NC

Marseilles can only reach SPA/SC, but can still support move to SPA/NC
Result: Support is VALID
```

---

## Retreat Phase

### When It Occurs
After movement phase, if any units are dislodged.

### Valid Retreat Destinations
A dislodged unit can retreat to a location that is:
1. Adjacent to its original location
2. Not occupied by another unit
3. Not the location the attacker came from
4. Not a location that was contested (multiple units tried to move there)

### Contested Location Rule
```
A PAR - BUR (bounced)
A MUN - BUR (bounced)
A MAR dislodged, tries to retreat

Result: A MAR cannot retreat to BUR (was contested)
```

### Convoy Didn't Happen = Not Contested (DATC 6.F.7-8)
```
A LON - HOL via convoy (convoy disrupted)
F NTH dislodged

Result: F NTH CAN retreat to HOL (convoy never actually contested it)
```

### Multiple Retreats to Same Location
If multiple dislodged units retreat to the same location, ALL disband.

### No Valid Retreats
If a unit has no valid retreat options, it auto-disbands.

---

## Adjustment Phase

### When It Occurs
After Fall Retreat phase, in Winter.

### Counting
```
adjustment = num_supply_centers - num_units

IF adjustment > 0: Can BUILD (up to adjustment) new units
IF adjustment < 0: Must DISBAND (abs(adjustment)) units
IF adjustment == 0: No action needed
```

### Build Restrictions
Can only build at:
1. Home supply centers
2. That are currently owned
3. That are empty (no unit present)

### Fleet Build at Split Coast
When building a fleet at a split coast location (STP, BUL, SPA), MUST specify coast.
```
F STP B      # VOID - must specify coast
F STP/SC B   # Valid
```

### Civil Disorder (Welfare Diplomacy)
In Welfare variant, if a power doesn't submit orders, units are disbanded by distance from home centers (furthest first).

---

## Result Codes

| Code | Value | Meaning |
|------|-------|---------|
| NONE | 0 | Not yet resolved |
| SUCCESS/OK | 1 | Order succeeded |
| BOUNCE | 2 | Move bounced (equal opposing force) |
| CUT | 3 | Support was cut by attacker |
| DISLODGED | 4 | Unit was dislodged by stronger attacker |
| VOID | 5 | Order is invalid (wrong syntax, impossible move) |
| FAILED | 6 | Order failed (generic failure) |
| NO_CONVOY | 7 | Convoy was disrupted or not used |
| DISRUPTED | 8 | Convoy disrupted by paradox resolution |
| DISBAND | 9 | Unit disbanded (retreat phase or adjustment) |

---

## Key DATC Test Categories

### 6.A - Basic Validity (12 tests)
Moving to non-adjacent, army to sea, fleet to land, ordering other power's units.

### 6.B - Coastal Issues (14 tests)
Split coast specification, coast inference, fleet coast adjacencies.

### 6.C - Circular Movement (7 tests)
2-way and 3-way cycles, circular movement with convoy.

### 6.D - Supports & Dislodges (34 tests)
Support cutting, support validation, dislodgement rules.

### 6.E - Head-to-Head (15 tests)
Direct battles, beleaguered garrison, self-dislodgement prevention.

### 6.F - Convoys (24 tests)
Basic convoys, multi-route, paradoxes, Szykman rule.

### 6.G - Adjacent Convoys (18 tests)
Kidnapping rule, intent-based convoy selection.

### 6.H - Retreats (16 tests)
Valid destinations, contested locations, convoy interaction.

### 6.I - Building (7 tests)
Build validation, home center requirements.

### 6.J - Civil Disorder (11 tests)
Auto-disbanding, welfare rules.

---

## Implementation Priority

Based on current failures (110/160), focus areas:

1. **Beleaguered Garrison Logic** (6.E.4-8, 6.E.10)
   - Non-dislodged loser still contests destination
   - Self-dislodgement prevention with ally support

2. **Convoy Paradox Resolution** (6.F.16-24)
   - DISRUPTED vs NO_CONVOY result codes
   - Szykman rule edge cases

3. **Adjacent Convoy Intent** (6.G.2-18)
   - Same-power convoy = voluntary
   - Enemy convoy = ignored (use land route)

4. **Contested Location After Disrupted Convoy** (6.F.7-8)
   - Disrupted convoy doesn't contest destination
   - Destination available for retreats

---

*Last Updated: December 28, 2025*
