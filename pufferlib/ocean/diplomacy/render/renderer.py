"""
Raylib-based renderer for Welfare Diplomacy.

This renderer visualizes the game state including:
- Background map
- Units (armies and fleets)
- Supply center ownership
- Orders (arrows, support lines, etc.)
- HUD (phase, year, welfare points)
"""

import math
import numpy as np
from cffi import FFI

try:
    from raylib import rl, colors
    import pyray
    RAYLIB_AVAILABLE = True
except ImportError:
    RAYLIB_AVAILABLE = False

from .map_data import (
    MAP_WIDTH, MAP_HEIGHT, UNIT_RADIUS, ARROW_HEAD_SIZE, LINE_WIDTH,
    PROVINCE_COORDS, DISLODGED_COORDS, POWER_COLORS, POWER_NAMES,
    PHASE_NAMES, SUPPLY_CENTERS, LOC_INDEX_TO_NAME,
    get_coords, get_coords_by_index, get_power_color, get_location_name,
)

# Background colors
PUFF_BACKGROUND = [6, 24, 24, 255]
PUFF_TEXT = [0, 187, 187, 255]

# Sea color for background
SEA_COLOR = (64, 128, 192, 255)
LAND_COLOR = (200, 180, 160, 255)


def cdata_to_numpy():
    """Convert Raylib screen to numpy array."""
    image = rl.LoadImageFromScreen()
    data_pointer = image.data
    width = image.width
    height = image.height
    channels = 4
    data_size = width * height * channels
    cdata = FFI().buffer(data_pointer, data_size)
    arr = np.frombuffer(cdata, dtype=np.uint8).reshape((height, width, channels)).copy()
    rl.UnloadImage(image)
    return arr


class DiplomacyRenderer:
    """
    Raylib-based renderer for Welfare Diplomacy.

    Usage:
        renderer = DiplomacyRenderer(env_handle, screen_width=1200, screen_height=900)
        while not renderer.should_close():
            renderer.render(show_orders=True)
        renderer.close()
    """

    def __init__(self, env_handle, screen_width=1200, screen_height=900, fps=30):
        """
        Initialize renderer with C environment handle.

        Args:
            env_handle: Handle from binding.env_init()
            screen_width: Window width in pixels
            screen_height: Window height in pixels
            fps: Target frames per second
        """
        if not RAYLIB_AVAILABLE:
            raise ImportError("raylib not available. Install with: pip install raylib")

        self.env_handle = env_handle
        self.screen_width = screen_width
        self.screen_height = screen_height
        self.fps = fps

        # Import binding here to avoid circular imports
        from pufferlib.ocean.diplomacy import binding
        self.binding = binding

        # Initialize Raylib window
        rl.InitWindow(screen_width, screen_height, b"Welfare Diplomacy")
        rl.SetTargetFPS(fps)

        # Camera for pan/zoom
        self.camera = pyray.Camera2D()
        self.camera.target = pyray.Vector2(MAP_WIDTH / 2, MAP_HEIGHT / 2)
        self.camera.offset = pyray.Vector2(screen_width / 2, screen_height / 2)
        self.camera.rotation = 0.0
        self.camera.zoom = min(screen_width / MAP_WIDTH, screen_height / MAP_HEIGHT) * 0.9

        # UI state
        self.paused = False
        self.show_help = False
        self.show_orders = True
        self.show_labels = True
        self.speed = min(screen_width, screen_height) / 100

        # Load map texture if available
        self.map_texture = None
        self._try_load_map_texture()

    def _try_load_map_texture(self):
        """Try to load the background map texture."""
        import os
        # Look for map in assets folder
        render_dir = os.path.dirname(os.path.abspath(__file__))
        map_path = os.path.join(render_dir, "assets", "map_bg.png")

        if os.path.exists(map_path):
            self.map_texture = rl.LoadTexture(map_path.encode())

    def should_close(self):
        """Check if window should close."""
        return rl.WindowShouldClose()

    def close(self):
        """Close the renderer and cleanup."""
        if self.map_texture:
            rl.UnloadTexture(self.map_texture)
        rl.CloseWindow()

    def _handle_input(self):
        """Handle keyboard/mouse input for camera controls."""
        screen_width = rl.GetScreenWidth()
        screen_height = rl.GetScreenHeight()

        fps = rl.GetFPS() or self.fps
        fps_mul = self.fps / fps
        speed = self.speed * fps_mul
        zoom_speed = 0.02 * fps_mul

        # Turbo mode
        if rl.IsKeyDown(rl.KEY_LEFT_SHIFT) or rl.IsKeyDown(rl.KEY_RIGHT_SHIFT):
            speed *= 3
            zoom_speed *= 3

        speed = speed / self.camera.zoom

        # Camera movement
        if rl.IsKeyDown(rl.KEY_UP) or rl.IsKeyDown(rl.KEY_W):
            self.camera.target.y -= speed
        if rl.IsKeyDown(rl.KEY_DOWN) or rl.IsKeyDown(rl.KEY_S):
            self.camera.target.y += speed
        if rl.IsKeyDown(rl.KEY_LEFT) or rl.IsKeyDown(rl.KEY_A):
            self.camera.target.x -= speed
        if rl.IsKeyDown(rl.KEY_RIGHT) or rl.IsKeyDown(rl.KEY_D):
            self.camera.target.x += speed

        # Zoom
        if rl.IsKeyDown(rl.KEY_Q) or rl.IsKeyDown(rl.KEY_MINUS):
            self.camera.zoom /= 1 + zoom_speed
        if rl.IsKeyDown(rl.KEY_E) or rl.IsKeyDown(rl.KEY_EQUAL):
            self.camera.zoom *= 1 + zoom_speed

        # Reset view
        if rl.IsKeyPressed(rl.KEY_SPACE):
            self.camera.zoom = min(screen_width / MAP_WIDTH, screen_height / MAP_HEIGHT) * 0.9
            self.camera.target.x = MAP_WIDTH / 2
            self.camera.target.y = MAP_HEIGHT / 2

        # Toggle help
        if rl.IsKeyPressed(rl.KEY_TAB) or rl.IsKeyPressed(rl.KEY_GRAVE):
            self.show_help = not self.show_help

        # Toggle orders
        if rl.IsKeyPressed(rl.KEY_O):
            self.show_orders = not self.show_orders

        # Toggle labels
        if rl.IsKeyPressed(rl.KEY_L):
            self.show_labels = not self.show_labels

        # Pause
        if rl.IsKeyPressed(rl.KEY_P):
            self.paused = not self.paused

        # Exit
        if rl.IsKeyDown(rl.KEY_ESCAPE):
            return True

        return False

    def render(self, show_orders=None, show_welfare=True):
        """
        Render current game state.

        Args:
            show_orders: Override for showing orders (None uses internal state)
            show_welfare: Whether to show welfare points in HUD

        Returns:
            numpy array of RGB pixels if needed for recording
        """
        if show_orders is not None:
            self.show_orders = show_orders

        # Handle input
        should_exit = self._handle_input()
        if should_exit:
            return None

        # Query game state from C
        game_state = self.binding.query_game_state(self.env_handle)

        # Begin drawing
        rl.BeginDrawing()
        rl.ClearBackground(SEA_COLOR)
        rl.BeginMode2D(self.camera)

        # Draw background map if available
        if self.map_texture:
            rl.DrawTexture(self.map_texture, 0, 0, colors.WHITE)
        else:
            # Draw simplified background (just a rectangle)
            rl.DrawRectangle(0, 0, MAP_WIDTH, MAP_HEIGHT, SEA_COLOR)

        # Draw supply center markers
        self._draw_supply_centers(game_state)

        # Draw province labels if enabled
        if self.show_labels:
            self._draw_labels()

        # Draw units
        self._draw_units(game_state)

        # Draw orders if enabled
        if self.show_orders:
            self._draw_orders(game_state)

        rl.EndMode2D()

        # Draw HUD (screen space)
        self._draw_hud(game_state, show_welfare)

        # Draw help overlay
        if self.show_help:
            self._draw_help()

        rl.EndDrawing()

        return cdata_to_numpy()

    def _draw_supply_centers(self, game_state):
        """Draw supply center ownership markers."""
        # Build ownership map from game state
        ownership = {}
        for power_id, power_data in enumerate(game_state['powers']):
            for center_idx in power_data['centers']:
                center_name = get_location_name(center_idx)
                ownership[center_name] = power_id

        for sc_name in SUPPLY_CENTERS:
            x, y = get_coords(sc_name)
            if x == 0 and y == 0:
                continue

            # Get owner color
            power_id = ownership.get(sc_name, -1)
            color = get_power_color(power_id)

            # Draw supply center as small square
            size = 8
            rl.DrawRectangle(
                int(x - size / 2), int(y - size / 2 - 20),
                size, size, color
            )
            rl.DrawRectangleLines(
                int(x - size / 2), int(y - size / 2 - 20),
                size, size, (0, 0, 0, 255)
            )

    def _draw_labels(self):
        """Draw province name labels."""
        for name, (x, y) in PROVINCE_COORDS.items():
            if "/" in name:  # Skip coast variants for labels
                continue
            # Draw small label below unit position
            label = name[:3]
            rl.DrawText(label.encode(), int(x - 10), int(y + 18), 10, (100, 100, 100, 200))

    def _draw_units(self, game_state):
        """Draw all units on the map."""
        for power_id, power_data in enumerate(game_state['powers']):
            color = get_power_color(power_id)
            for unit in power_data['units']:
                loc_idx = unit['location']
                loc_name = get_location_name(loc_idx)
                x, y = get_coords(loc_name)

                if x == 0 and y == 0:
                    continue

                if unit['type'] == 1:  # UNIT_ARMY
                    self._draw_army(x, y, color)
                else:  # UNIT_FLEET
                    self._draw_fleet(x, y, color)

    def _draw_army(self, x, y, color):
        """Draw army symbol (filled circle with border)."""
        # Shadow
        rl.DrawCircle(int(x + 2), int(y + 2), UNIT_RADIUS, (0, 0, 0, 100))
        # Main circle
        rl.DrawCircle(int(x), int(y), UNIT_RADIUS, color)
        # Border
        rl.DrawCircleLines(int(x), int(y), UNIT_RADIUS, (0, 0, 0, 255))
        # Inner detail (army symbol = circle with center dot)
        rl.DrawCircle(int(x), int(y), 4, (0, 0, 0, 128))

    def _draw_fleet(self, x, y, color):
        """Draw fleet symbol (diamond/rhombus shape)."""
        # Shadow
        size = UNIT_RADIUS
        self._draw_diamond(x + 2, y + 2, size, (0, 0, 0, 100))
        # Main shape
        self._draw_diamond(x, y, size, color)
        # Border
        self._draw_diamond_lines(x, y, size, (0, 0, 0, 255))

    def _draw_diamond(self, x, y, size, color):
        """Draw filled diamond shape."""
        points = [
            (x, y - size),      # Top
            (x + size, y),      # Right
            (x, y + size),      # Bottom
            (x - size, y),      # Left
        ]
        # Draw as two triangles
        rl.DrawTriangle(
            pyray.Vector2(points[0][0], points[0][1]),
            pyray.Vector2(points[1][0], points[1][1]),
            pyray.Vector2(points[2][0], points[2][1]),
            color
        )
        rl.DrawTriangle(
            pyray.Vector2(points[0][0], points[0][1]),
            pyray.Vector2(points[2][0], points[2][1]),
            pyray.Vector2(points[3][0], points[3][1]),
            color
        )

    def _draw_diamond_lines(self, x, y, size, color):
        """Draw diamond outline."""
        rl.DrawLine(int(x), int(y - size), int(x + size), int(y), color)
        rl.DrawLine(int(x + size), int(y), int(x), int(y + size), color)
        rl.DrawLine(int(x), int(y + size), int(x - size), int(y), color)
        rl.DrawLine(int(x - size), int(y), int(x), int(y - size), color)

    def _draw_orders(self, game_state):
        """Draw order visualizations (arrows, support lines, etc.)."""
        # Orders are not directly exposed in query_game_state yet
        # This will be implemented in order_viz.py
        pass

    def _draw_hud(self, game_state, show_welfare):
        """Draw heads-up display (phase, year, welfare points)."""
        year = game_state['year']
        phase = game_state['phase']
        phase_name = PHASE_NAMES.get(phase, f"Phase {phase}")

        # Phase and year (top left)
        phase_text = f"{phase_name} {year}"
        rl.DrawRectangle(5, 5, 200, 30, (0, 0, 0, 180))
        rl.DrawText(phase_text.encode(), 10, 10, 20, PUFF_TEXT)

        # Power stats (right side)
        y_offset = 40
        for power_id, power_data in enumerate(game_state['powers']):
            color = get_power_color(power_id)
            name = POWER_NAMES[power_id][:3]
            centers = power_data['num_centers']
            units = power_data['num_units']
            welfare = power_data['welfare_points']

            # Background
            rl.DrawRectangle(self.screen_width - 145, y_offset, 140, 22, (0, 0, 0, 150))

            # Power indicator square
            rl.DrawRectangle(self.screen_width - 140, y_offset + 3, 16, 16, color)

            if show_welfare:
                stat_text = f"{name}: C{centers} U{units} W{welfare}"
            else:
                stat_text = f"{name}: C{centers} U{units}"

            rl.DrawText(stat_text.encode(), self.screen_width - 120, y_offset + 4, 14, PUFF_TEXT)
            y_offset += 24

    def _draw_help(self):
        """Draw help overlay with keyboard controls."""
        # Semi-transparent background
        rl.DrawRectangle(10, 50, 250, 200, (0, 0, 0, 200))

        y = 55
        spacing = 18
        rl.DrawText(b"=== Controls ===", 15, y, 16, PUFF_TEXT); y += spacing + 5
        rl.DrawText(b"WASD/Arrows: Pan", 15, y, 14, PUFF_TEXT); y += spacing
        rl.DrawText(b"Q/E or -/+: Zoom", 15, y, 14, PUFF_TEXT); y += spacing
        rl.DrawText(b"Space: Reset view", 15, y, 14, PUFF_TEXT); y += spacing
        rl.DrawText(b"Shift: Turbo", 15, y, 14, PUFF_TEXT); y += spacing
        rl.DrawText(b"Tab/~: Toggle help", 15, y, 14, PUFF_TEXT); y += spacing
        rl.DrawText(b"O: Toggle orders", 15, y, 14, PUFF_TEXT); y += spacing
        rl.DrawText(b"L: Toggle labels", 15, y, 14, PUFF_TEXT); y += spacing
        rl.DrawText(b"P: Pause", 15, y, 14, PUFF_TEXT); y += spacing
        rl.DrawText(b"ESC: Exit", 15, y, 14, PUFF_TEXT)


def create_renderer(env_handle, **kwargs):
    """Factory function to create a DiplomacyRenderer.

    Args:
        env_handle: Handle from binding.env_init()
        **kwargs: Additional arguments passed to DiplomacyRenderer

    Returns:
        DiplomacyRenderer instance
    """
    return DiplomacyRenderer(env_handle, **kwargs)
