"""Legacy environment namespace retained for Diplomacy rule-test access.

The PufferLib 4 trainer uses pufferlib.models, not the old Ocean Torch policies.
Import a specific environment explicitly; do not load unrelated 3.0 modules.
"""
