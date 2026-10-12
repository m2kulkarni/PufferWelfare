"""PufferLib 4 trainer source with an explicit legacy test-wrapper bridge."""
__version__ = "4.0.0.dev0"


def __getattr__(name):
    if name == "PufferEnv":
        # Kept only for the legacy Diplomacy adjudication wrapper.
        from pufferlib.pufferlib import PufferEnv
        return PufferEnv
    raise AttributeError(f"module {__name__!r} has no attribute {name!r}")
