import numpy as np
import gymnasium
from gymnasium import spaces
import pufferlib
from pufferlib.ocean.diplomacy import binding


class Diplomacy(pufferlib.PufferEnv):
    """
    Welfare Diplomacy environment for PufferLib.

    A multi-agent general-sum variant of the board game Diplomacy where:
    - Powers can voluntarily disband units
    - Welfare points = cumulative (centers - units) after each adjustment phase
    - Game ends after fixed number of years (no victory condition)
    - Final utility = total welfare points accumulated

    This is a C/C++ implementation for high-performance RL training.
    """

    def __init__(
        self,
        max_years=10,
        welfare_mode=True,
        num_players=7,
        render_mode=None,
        buf=None,
        seed=1,
        **kwargs,
    ):
        # Handle string params from config files
        if isinstance(max_years, str):
            max_years = int(max_years)
        if isinstance(welfare_mode, str):
            welfare_mode = welfare_mode.lower() in ('true', '1', 'yes')

        if type(max_years) is not int or not 1 <= max_years <= 1000:
            raise ValueError("max_years must be an integer between 1 and 1000")
        if type(welfare_mode) is not bool or num_players != 7:
            raise ValueError("Diplomacy requires seven powers and a boolean welfare_mode")
        self.max_years = max_years
        self.welfare_mode = welfare_mode
        self.num_players = num_players
        self.render_mode = render_mode
        self.seed = seed

        # Observation space: 81 locations × 20 features + 21 global = 1641
        # Per location: unit_type(3) + unit_owner(8) + sc_owner(8) + buildable(1) = 20
        # Global: phase(6) + year(1) + build_delta(7) + welfare(7) = 21
        self.single_observation_space = spaces.Box(
            low=0, high=1, shape=(1641,), dtype=np.float32
        )

        # Action space: 17 unit slots × 64 order types = 1088
        # action = unit_index * 64 + order_type
        self.single_action_space = spaces.Discrete(1088)

        # Number of agents (required by PufferEnv)
        self.num_agents = num_players

        # Call parent init (sets up buffers via set_buffers)
        super().__init__(buf)

        # Initialize C environment handle
        self.env_handle = binding.env_init(
            self.observations,
            self.actions,
            self.rewards,
            self.terminals,
            self.truncations,
            seed
        )

        # Configure runtime settings in C
        try:
            binding.env_configure(self.env_handle, int(self.welfare_mode), int(self.max_years))
        except AttributeError:
            # Backward compatibility if binding not yet exposes configure
            pass

        # Renderer (lazy initialization)
        self._renderer = None

    def reset(self, seed=None):
        """Reset the environment to initial state."""
        self._require_open()
        if seed is not None:
            self.seed = seed

        # Call C reset function
        binding.env_reset(self.env_handle, self.seed)

        # Observations are updated in self.observations by C code
        # Return empty info list (one per agent)
        return self.observations, []

    def step(self, actions):
        """Execute one step of the environment."""
        self._require_open()
        values = np.asarray(actions)
        if values.shape != self.actions.shape or not np.issubdtype(values.dtype, np.number):
            raise ValueError("actions must contain one integer per power")
        if not np.isfinite(values).all() or (values != np.floor(values)).any() or (values < 0).any() or (values >= 1088).any():
            raise ValueError("actions must be integers in [0, 1088)")
        self.actions[:] = values
        # Copy arguments into the shared action buffer before stepping.
        # Call C step function which will process them
        binding.env_step(self.env_handle)

        # All results are updated in place in the buffers by C code
        # Return empty info list (one per agent)
        return self.observations, self.rewards, self.terminals, self.truncations, []

    def render(self):
        """Render the current game state.

        Returns:
            numpy array of RGB pixels if render_mode is 'rgb_array',
            None otherwise.
        """
        if self.render_mode == "human":
            if self._renderer is None:
                from .render import DiplomacyRenderer
                self._renderer = DiplomacyRenderer(self.env_handle)
            return self._renderer.render()
        elif self.render_mode == "rgb_array":
            if self._renderer is None:
                from .render import DiplomacyRenderer
                self._renderer = DiplomacyRenderer(self.env_handle)
            return self._renderer.render()
        return None

    def close(self):
        """Clean up resources."""
        if getattr(self, '_renderer', None) is not None:
            self._renderer.close()
            self._renderer = None
        if getattr(self, 'env_handle', None) is not None:
            binding.env_close(self.env_handle)
            self.env_handle = None

    def _require_open(self):
        if getattr(self, 'env_handle', None) is None:
            raise RuntimeError("Diplomacy environment is closed")

    def __del__(self):
        self.close()
