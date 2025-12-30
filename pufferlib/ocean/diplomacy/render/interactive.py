"""
Interactive controls for human play in Welfare Diplomacy.

Allows human players to:
- Select units by clicking
- Issue orders (move, hold, support, convoy)
- Submit orders and advance phases
"""

import math

try:
    from raylib import rl
    import pyray
    RAYLIB_AVAILABLE = True
except ImportError:
    RAYLIB_AVAILABLE = False

from .map_data import (
    MAP_WIDTH, MAP_HEIGHT, UNIT_RADIUS, PROVINCE_COORDS,
    LOC_INDEX_TO_NAME, LOC_NAME_TO_INDEX,
    get_coords, get_coords_by_index, get_power_color, get_location_name,
    POWER_NAMES,
)


class InteractiveController:
    """
    Handles human player interaction for order submission.
    """

    def __init__(self, renderer, env_handle, binding):
        """Initialize interactive controller.

        Args:
            renderer: DiplomacyRenderer instance
            env_handle: C environment handle
            binding: diplomacy binding module
        """
        self.renderer = renderer
        self.env_handle = env_handle
        self.binding = binding

        # Current state
        self.selected_unit = None  # {"power_id": int, "type": int, "location": str}
        self.pending_orders = {}   # power_id -> [order_strings]
        self.current_power = 0     # Power ID being controlled
        self.order_mode = None     # None, "move", "support", "convoy"

    def set_current_power(self, power_id):
        """Set which power the human is controlling.

        Args:
            power_id: Power ID (0-6)
        """
        self.current_power = power_id
        self.selected_unit = None
        self.order_mode = None

    def next_power(self):
        """Switch to next power."""
        self.current_power = (self.current_power + 1) % 7
        self.selected_unit = None
        self.order_mode = None

    def handle_input(self, game_state):
        """Handle mouse and keyboard input.

        Args:
            game_state: Current game state dict from query_game_state()

        Returns:
            True if an order was submitted, False otherwise
        """
        # Keyboard shortcuts
        if rl.IsKeyPressed(rl.KEY_H):
            # Hold order
            if self.selected_unit:
                self._issue_hold()
                return True

        if rl.IsKeyPressed(rl.KEY_M):
            # Move mode
            self.order_mode = "move"

        if rl.IsKeyPressed(rl.KEY_C):
            # Convoy mode
            self.order_mode = "convoy"

        if rl.IsKeyPressed(rl.KEY_R):
            # Support mode
            self.order_mode = "support"

        if rl.IsKeyPressed(rl.KEY_ENTER) or rl.IsKeyPressed(rl.KEY_KP_ENTER):
            # Submit all orders
            self.submit_orders()
            return True

        if rl.IsKeyPressed(rl.KEY_N):
            # Next power
            self.next_power()

        if rl.IsKeyPressed(rl.KEY_BACKSPACE):
            # Clear selection
            self.selected_unit = None
            self.order_mode = None

        if rl.IsKeyPressed(rl.KEY_DELETE):
            # Clear pending orders for current power
            if self.current_power in self.pending_orders:
                self.pending_orders[self.current_power] = []

        # Mouse click
        if rl.IsMouseButtonPressed(rl.MOUSE_LEFT_BUTTON):
            # Get mouse position in world coordinates
            screen_pos = rl.GetMousePosition()
            world_pos = rl.GetScreenToWorld2D(screen_pos, self.renderer.camera)

            # Find clicked province
            clicked_loc = self._find_province_at(world_pos.x, world_pos.y)

            if clicked_loc:
                if self.selected_unit is None:
                    # Try to select a unit
                    self._try_select_unit(clicked_loc, game_state)
                else:
                    # Issue order to selected unit
                    self._issue_order(clicked_loc)
                    return True

        return False

    def _find_province_at(self, x, y):
        """Find province at given world coordinates.

        Args:
            x, y: World coordinates

        Returns:
            Province name or None if no province found
        """
        # Find closest province within threshold
        min_dist = float('inf')
        closest = None
        threshold = 30  # pixels

        for name, (px, py) in PROVINCE_COORDS.items():
            dist = math.sqrt((x - px) ** 2 + (y - py) ** 2)
            if dist < threshold and dist < min_dist:
                min_dist = dist
                closest = name

        return closest

    def _try_select_unit(self, loc, game_state):
        """Try to select a unit at the given location.

        Args:
            loc: Location name
            game_state: Current game state
        """
        power_data = game_state['powers'][self.current_power]

        for unit in power_data['units']:
            unit_loc_idx = unit['location']
            unit_loc = get_location_name(unit_loc_idx)

            # Check if unit is at this location (handle coast variants)
            if unit_loc.upper() == loc.upper():
                self.selected_unit = {
                    'power_id': self.current_power,
                    'type': unit['type'],
                    'location': unit_loc,
                }
                self.order_mode = "move"  # Default to move mode
                return

            # Also check parent location for coast variants
            if "/" in unit_loc:
                parent = unit_loc.split("/")[0]
                if parent.upper() == loc.upper():
                    self.selected_unit = {
                        'power_id': self.current_power,
                        'type': unit['type'],
                        'location': unit_loc,
                    }
                    self.order_mode = "move"
                    return

    def _issue_hold(self):
        """Issue hold order for selected unit."""
        if self.selected_unit is None:
            return

        unit_type = 'A' if self.selected_unit['type'] == 1 else 'F'
        loc = self.selected_unit['location']
        order = f"{unit_type} {loc} H"

        self._add_pending_order(order)
        self.selected_unit = None
        self.order_mode = None

    def _issue_order(self, dest_loc):
        """Issue order from selected unit to destination.

        Args:
            dest_loc: Destination location name
        """
        if self.selected_unit is None:
            return

        unit_type = 'A' if self.selected_unit['type'] == 1 else 'F'
        from_loc = self.selected_unit['location']

        if self.order_mode == "move":
            order = f"{unit_type} {from_loc} - {dest_loc}"
        elif self.order_mode == "support":
            # Simplified support - just support hold at destination
            order = f"{unit_type} {from_loc} S {dest_loc}"
        elif self.order_mode == "convoy":
            # Simplified convoy - this needs more work for proper implementation
            order = f"{unit_type} {from_loc} C {dest_loc}"
        else:
            # Default to move
            order = f"{unit_type} {from_loc} - {dest_loc}"

        self._add_pending_order(order)
        self.selected_unit = None
        self.order_mode = None

    def _add_pending_order(self, order):
        """Add order to pending orders.

        Args:
            order: Order string (e.g., "A PAR - BUR")
        """
        if self.current_power not in self.pending_orders:
            self.pending_orders[self.current_power] = []
        self.pending_orders[self.current_power].append(order)

    def submit_orders(self):
        """Submit all pending orders to the C engine."""
        for power_id, orders in self.pending_orders.items():
            if orders:
                self.binding.game_submit_orders(self.env_handle, power_id, orders)
        self.pending_orders.clear()

    def get_pending_orders(self):
        """Get all pending orders.

        Returns:
            Dict of power_id -> [order_strings]
        """
        return self.pending_orders

    def draw_selection(self):
        """Draw selection indicator for selected unit."""
        if self.selected_unit is None:
            return

        loc = self.selected_unit['location']
        x, y = get_coords(loc)
        if (x, y) == (0, 0):
            return

        # Draw pulsing selection circle
        color = get_power_color(self.current_power)
        rl.DrawCircleLines(int(x), int(y), UNIT_RADIUS + 8, color)
        rl.DrawCircleLines(int(x), int(y), UNIT_RADIUS + 10, (255, 255, 255, 200))

    def draw_pending_orders(self):
        """Draw pending orders that haven't been submitted yet."""
        from .order_viz import OrderVisualizer
        viz = OrderVisualizer()

        for power_id, orders in self.pending_orders.items():
            color = get_power_color(power_id)
            # Make pending orders semi-transparent
            pending_color = (color[0], color[1], color[2], 150)

            for order_str in orders:
                self._draw_order_from_string(viz, order_str, pending_color)

    def _draw_order_from_string(self, viz, order_str, color):
        """Draw an order from its string representation.

        Args:
            viz: OrderVisualizer instance
            order_str: Order string (e.g., "A PAR - BUR")
            color: RGB tuple for the order color
        """
        parts = order_str.split()
        if len(parts) < 3:
            return

        unit_type = parts[0]
        from_loc = parts[1]

        if parts[2] == 'H':
            # Hold
            viz.draw_hold(from_loc, color)
        elif parts[2] == '-':
            # Move
            if len(parts) >= 4:
                to_loc = parts[3]
                viz.draw_move(from_loc, to_loc, color)
        elif parts[2] == 'S':
            # Support
            if len(parts) >= 4:
                target_loc = parts[3]
                viz.draw_support_hold(from_loc, target_loc, color)
        elif parts[2] == 'C':
            # Convoy
            if len(parts) >= 4:
                army_loc = parts[3]
                viz.draw_convoy(from_loc, army_loc, army_loc, color)

    def draw_ui(self):
        """Draw interactive UI overlay."""
        # Current power indicator
        screen_width = self.renderer.screen_width
        power_name = POWER_NAMES[self.current_power]
        color = get_power_color(self.current_power)

        rl.DrawRectangle(screen_width - 200, 10, 190, 30, (0, 0, 0, 200))
        rl.DrawRectangle(screen_width - 195, 15, 20, 20, color)
        rl.DrawText(f"Playing: {power_name}".encode(), screen_width - 170, 17, 14, (0, 187, 187, 255))

        # Order mode indicator
        if self.order_mode:
            mode_text = f"Mode: {self.order_mode.upper()}"
            rl.DrawRectangle(screen_width - 200, 45, 190, 25, (0, 0, 0, 200))
            rl.DrawText(mode_text.encode(), screen_width - 195, 50, 14, (0, 187, 187, 255))

        # Selected unit
        if self.selected_unit:
            sel_text = f"Selected: {self.selected_unit['location']}"
            rl.DrawRectangle(screen_width - 200, 75, 190, 25, (0, 0, 0, 200))
            rl.DrawText(sel_text.encode(), screen_width - 195, 80, 14, (255, 255, 0, 255))

        # Pending orders count
        total_pending = sum(len(orders) for orders in self.pending_orders.values())
        if total_pending > 0:
            pending_text = f"Pending orders: {total_pending}"
            rl.DrawRectangle(screen_width - 200, 105, 190, 25, (0, 0, 0, 200))
            rl.DrawText(pending_text.encode(), screen_width - 195, 110, 14, (0, 255, 0, 255))


def create_interactive_controller(renderer, env_handle, binding):
    """Factory function to create an InteractiveController.

    Args:
        renderer: DiplomacyRenderer instance
        env_handle: C environment handle
        binding: diplomacy binding module

    Returns:
        InteractiveController instance
    """
    return InteractiveController(renderer, env_handle, binding)
