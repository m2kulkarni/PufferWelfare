# Welfare Diplomacy Training Plan

## Overview

This document describes how to train a neural network to play Welfare Diplomacy using PufferLib.

**Welfare Diplomacy differences from standard:**
- Powers can voluntarily disband units
- Welfare points = cumulative (centers - units) after each adjustment phase
- Game ends after fixed years (no elimination victory)
- Goal: maximize total welfare points accumulated

---

## Observation Encoding

### Board Representation: 81 Locations

The Diplomacy map has **81 locations**:
- 75 land/sea provinces
- 6 coast variants (SPA/NC, SPA/SC, STP/NC, STP/SC, BUL/EC, BUL/SC)

### Features Per Location: 20 values

For each of the 81 locations, we encode:

| Index | Feature | Values | Description |
|-------|---------|--------|-------------|
| 0-2 | Unit type | 3 | One-hot: [Army, Fleet, Empty] |
| 3-10 | Unit owner | 8 | One-hot: [Austria, England, France, Germany, Italy, Russia, Turkey, None] |
| 11-18 | SC owner | 8 | One-hot: [Austria, England, France, Germany, Italy, Russia, Turkey, Neutral] |
| 19 | Buildable | 1 | 1 if home SC and empty, else 0 |

**Per-location total: 20 features**

### Global Features: 21 values

| Index | Feature | Values | Description |
|-------|---------|--------|-------------|
| 0-5 | Phase | 6 | One-hot: [Spring Move, Spring Retreat, Fall Move, Fall Retreat, Winter Adjust, Complete] |
| 6 | Year | 1 | Normalized: (year - 1901) / max_years |
| 7-13 | Build delta | 7 | Per power: (centers - units) / 10, clamped to [-1, 1] |
| 14-20 | Welfare | 7 | Per power: welfare_points / 100, clamped to [0, 1] |

**Global total: 21 features**

### Total Observation Shape

```
Board:  81 locations × 20 features = 1,620
Global: 21 features
─────────────────────────────────────────
Total:  1,641 features

observation_space = Box(low=0, high=1, shape=(1641,), dtype=float32)
```

---

## Action Encoding

### Per-Power Action Space

Each power submits one action per step. The action encodes which unit to command and what order to give.

```
action = unit_index × 64 + order_type

Where:
- unit_index: 0-16 (max 17 units per power)
- order_type: 0-63 (order + target encoding)
```

**Order Types (0-63):**

| Range | Order | Description |
|-------|-------|-------------|
| 0 | HOLD | Unit holds position |
| 1-30 | MOVE | Move to adjacent location (index = adjacency number) |
| 31-50 | SUPPORT | Support another unit |
| 51-60 | CONVOY | Convoy order (fleets only) |
| 61 | DISBAND | Voluntarily disband (Welfare mode) |
| 62 | BUILD_ARMY | Build army (adjustment phase) |
| 63 | BUILD_FLEET | Build fleet (adjustment phase) |

### Total Action Space

```
17 unit slots × 64 order types = 1,088 actions

action_space = Discrete(1088)
```

**Note:** Invalid actions (e.g., ordering non-existent unit) default to HOLD.

---

## Neural Network Architecture (Simple MLP)

### Network Diagram

```
Input: observations [batch, 1641]
         │
         ▼
    ┌─────────────┐
    │  Linear     │  1641 → 512
    │  + ReLU     │
    └─────────────┘
         │
         ▼
    Shape: [batch, 512]
         │
         ▼
    ┌─────────────┐
    │  Linear     │  512 → 512
    │  + ReLU     │
    └─────────────┘
         │
         ▼
    Shape: [batch, 512]
         │
    ┌────┴────┐
    ▼         ▼
┌───────┐ ┌───────┐
│ Actor │ │Critic │
│Linear │ │Linear │
│512→   │ │512→1  │
│1088   │ │       │
└───────┘ └───────┘
    │         │
    ▼         ▼
[batch,    [batch, 1]
 1088]

Action     Value
Logits     Estimate
```

### Shape Summary

| Layer | Input Shape | Output Shape | Parameters |
|-------|-------------|--------------|------------|
| Input | - | [B, 1641] | 0 |
| Linear1 | [B, 1641] | [B, 512] | 1641×512 + 512 = 840,704 |
| ReLU | [B, 512] | [B, 512] | 0 |
| Linear2 | [B, 512] | [B, 512] | 512×512 + 512 = 262,656 |
| ReLU | [B, 512] | [B, 512] | 0 |
| Actor | [B, 512] | [B, 1088] | 512×1088 + 1088 = 558,144 |
| Critic | [B, 512] | [B, 1] | 512×1 + 1 = 513 |
| **Total** | | | **1,662,017** |

### PyTorch Implementation

```python
class Diplomacy(nn.Module):
    def __init__(self, env, hidden_size=512, **kwargs):
        super().__init__()
        self.is_continuous = False

        # Get shapes from environment
        obs_size = env.single_observation_space.shape[0]  # 1641
        num_actions = env.single_action_space.n           # 1088

        # Encoder: observations → hidden features
        self.encoder = nn.Sequential(
            nn.Linear(obs_size, hidden_size),   # 1641 → 512
            nn.ReLU(),
            nn.Linear(hidden_size, hidden_size), # 512 → 512
            nn.ReLU(),
        )

        # Actor: hidden → action logits
        self.actor = nn.Linear(hidden_size, num_actions)  # 512 → 1088

        # Critic: hidden → value estimate
        self.value_fn = nn.Linear(hidden_size, 1)  # 512 → 1

    def forward(self, observations, state=None):
        # observations: [batch, 1641]
        hidden = self.encoder(observations)  # [batch, 512]
        action_logits = self.actor(hidden)   # [batch, 1088]
        value = self.value_fn(hidden)        # [batch, 1]
        return action_logits, value
```

---

## Data Flow During Training

### Forward Pass

```
1. Environment provides observation
   obs = [1641 floats]

2. Policy network computes action distribution
   logits = policy(obs)  # [1088 floats]

3. Sample action from distribution
   action = Categorical(logits).sample()  # single int 0-1087

4. Decode action
   unit_idx = action // 64    # which unit (0-16)
   order_idx = action % 64    # which order (0-63)

5. Execute in environment
   env.step(action)
```

### Reward Signal (Welfare Mode)

```python
# Reward = change in welfare points
reward = current_welfare - previous_welfare

# Welfare calculated at end of each adjustment phase:
# welfare_delta = max(0, centers - units)
# welfare_points += welfare_delta
```

---

## Implementation Checklist

### C Code Changes

- [ ] Update `encode_observations()` to produce 1641 features
- [ ] Implement `decode_action()` to convert int → Order struct
- [ ] Modify `c_step()` to read actions buffer and apply orders
- [ ] Update observation space shape in Python

### Python Changes

- [ ] Update `single_observation_space` to Box(1641)
- [ ] Update `single_action_space` to Discrete(1088)
- [ ] Add `Diplomacy` policy class to `pufferlib/ocean/torch.py`
- [ ] Register `'diplomacy'` in `environment.py` MAKE_FUNCTIONS

### Testing

- [ ] Verify observation encoding produces correct shapes
- [ ] Verify action decoding produces valid orders
- [ ] Run training: `puffer train puffer_diplomacy`
