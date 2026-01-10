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
    from raylib import rl, colors, ffi
    RAYLIB_AVAILABLE = True
except ImportError:
    RAYLIB_AVAILABLE = False
    ffi = None

from .map_data import (
    MAP_WIDTH, MAP_HEIGHT, UNIT_RADIUS, FLEET_SIZE, ARROW_HEAD_SIZE, LINE_WIDTH,
    LABEL_FONT_SIZE, SC_SIZE,
    PROVINCE_COORDS, DISLODGED_COORDS, POWER_COLORS, POWER_UNIT_COLORS, POWER_NAMES,
    PHASE_NAMES, SUPPLY_CENTERS, LOC_INDEX_TO_NAME, LABEL_COORDS,
    get_coords, get_coords_by_index, get_power_color, get_location_name, get_power_unit_color,
)
from .province_polygons import PROVINCE_POLYGONS

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

        # Camera for pan/zoom (using ffi to create structs)
        self.camera = ffi.new('Camera2D *')
        self.camera.target = ffi.new('Vector2 *', [MAP_WIDTH / 2, MAP_HEIGHT / 2])[0]
        self.camera.offset = ffi.new('Vector2 *', [screen_width / 2, screen_height / 2])[0]
        self.camera.rotation = 0.0
        # Start with a zoom that fits the map nicely
        self.camera.zoom = min(screen_width / MAP_WIDTH, screen_height / MAP_HEIGHT)

        # UI state
        self.paused = False
        self.show_help = False
        self.show_orders = True
        self.show_labels = True
        self.speed = min(screen_width, screen_height) / 100

        # Load textures
        self.map_texture = None
        self.army_texture = None
        self.fleet_texture = None
        self._load_textures()

    def _load_textures(self):
        """Load all textures (map, unit symbols)."""
        import os
        render_dir = os.path.dirname(os.path.abspath(__file__))

        # Map background
        map_path = os.path.join(render_dir, "assets", "map_bg.png")
        if os.path.exists(map_path):
            self.map_texture = rl.LoadTexture(map_path.encode())
            # Use point filtering for crisp pixels (no blur)
            rl.SetTextureFilter(self.map_texture, rl.TEXTURE_FILTER_POINT)

        # Unit symbols
        army_path = os.path.join(render_dir, "assets", "army.png")
        if os.path.exists(army_path):
            self.army_texture = rl.LoadTexture(army_path.encode())
            rl.SetTextureFilter(self.army_texture, rl.TEXTURE_FILTER_POINT)

        fleet_path = os.path.join(render_dir, "assets", "fleet.png")
        if os.path.exists(fleet_path):
            self.fleet_texture = rl.LoadTexture(fleet_path.encode())
            rl.SetTextureFilter(self.fleet_texture, rl.TEXTURE_FILTER_POINT)

        # Load a proper font for crisp text
        self.font = None
        font_paths = [
            os.path.join(render_dir, "assets", "font.ttf"),
            "/usr/share/fonts/truetype/dejavu/DejaVuSans-Bold.ttf",
            "/usr/share/fonts/truetype/dejavu/DejaVuSans.ttf",
        ]
        for font_path in font_paths:
            if os.path.exists(font_path):
                self.font = rl.LoadFontEx(font_path.encode(), 48, ffi.NULL, 0)
                if self.font.glyphCount > 0:
                    rl.SetTextureFilter(self.font.texture, rl.TEXTURE_FILTER_BILINEAR)
                    break
                else:
                    self.font = None

    def should_close(self):
        """Check if window should close."""
        return rl.WindowShouldClose()

    def close(self):
        """Close the renderer and cleanup."""
        if self.map_texture:
            rl.UnloadTexture(self.map_texture)
        if self.army_texture:
            rl.UnloadTexture(self.army_texture)
        if self.fleet_texture:
            rl.UnloadTexture(self.fleet_texture)
        if self.font:
            rl.UnloadFont(self.font)
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
        rl.BeginMode2D(self.camera[0])  # Dereference the camera pointer

        # Draw background map texture
        if self.map_texture:
            rl.DrawTexture(self.map_texture, 0, 0, colors.WHITE)
        else:
            # Fallback: draw sea color
            rl.DrawRectangle(0, 0, MAP_WIDTH, MAP_HEIGHT, SEA_COLOR)

        # Draw province influence (colored overlays for owned territories)
        self._draw_province_influence(game_state)

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

    def render_to_image(self, scale=1.0):
        """
        Render high-quality image without camera transformation.

        Args:
            scale: Scale factor (1.0 = map resolution, 2.0 = 2x resolution)

        Returns:
            numpy array of RGBA pixels at full quality
        """
        width = int(MAP_WIDTH * scale)
        height = int(MAP_HEIGHT * scale)

        # Create a RenderTexture for off-screen rendering
        target = rl.LoadRenderTexture(width, height)
        rl.SetTextureFilter(target.texture, rl.TEXTURE_FILTER_POINT)

        # Query game state
        game_state = self.binding.query_game_state(self.env_handle)

        # Render to texture
        rl.BeginTextureMode(target)
        rl.ClearBackground(SEA_COLOR)

        # Draw background map texture
        if self.map_texture:
            if scale == 1.0:
                rl.DrawTexture(self.map_texture, 0, 0, colors.WHITE)
            else:
                src = ffi.new('Rectangle *', [0, 0, self.map_texture.width, self.map_texture.height])[0]
                dst = ffi.new('Rectangle *', [0, 0, width, height])[0]
                origin = ffi.new('Vector2 *', [0, 0])[0]
                rl.DrawTexturePro(self.map_texture, src, dst, origin, 0, colors.WHITE)
        else:
            rl.DrawRectangle(0, 0, width, height, SEA_COLOR)

        # Draw game elements (note: scaled rendering only affects map texture and labels)
        self._draw_province_influence(game_state)
        self._draw_supply_centers(game_state)
        if self.show_labels:
            font_size = int(LABEL_FONT_SIZE * scale) if scale != 1.0 else None
            self._draw_labels(font_size=font_size)
        self._draw_units(game_state)

        rl.EndTextureMode()

        # Convert RenderTexture to numpy array
        image = rl.LoadImageFromTexture(target.texture)
        # Flip vertically (RenderTexture is upside down)
        rl.ImageFlipVertical(ffi.addressof(image))

        data_pointer = image.data
        channels = 4
        data_size = width * height * channels
        cdata = FFI().buffer(data_pointer, data_size)
        arr = np.frombuffer(cdata, dtype=np.uint8).reshape((height, width, channels)).copy()

        rl.UnloadImage(image)
        rl.UnloadRenderTexture(target)

        return arr

    def _build_ownership_map(self, game_state):
        """Build a mapping of province names to owning power IDs."""
        ownership = {}
        for power_id, power_data in enumerate(game_state['powers']):
            for center_idx in power_data['centers']:
                center_name = get_location_name(center_idx)
                ownership[center_name] = power_id
        return ownership

    def _draw_province_influence(self, game_state):
        """Draw colored overlays for provinces owned by each power."""
        ownership = self._build_ownership_map(game_state)

        # Draw filled polygons for owned provinces
        for province_name, polygons in PROVINCE_POLYGONS.items():
            power_id = ownership.get(province_name, -1)
            if power_id >= 0:
                # Get power color - darker and more solid
                base_color = get_power_color(power_id)
                overlay_color = (base_color[0], base_color[1], base_color[2], 230)

                # Draw each polygon for this province
                for polygon in polygons:
                    if len(polygon) >= 3:
                        self._draw_filled_polygon(polygon, overlay_color)
                        # Draw black border
                        self._draw_polygon_outline(polygon, (0, 0, 0, 255), 2)

    def _draw_supply_centers(self, game_state):
        """Draw supply center ownership markers."""
        ownership = self._build_ownership_map(game_state)

        for sc_name in SUPPLY_CENTERS:
            x, y = get_coords(sc_name)
            if x == 0 and y == 0:
                continue

            # Get owner color
            power_id = ownership.get(sc_name, -1)
            color = get_power_color(power_id)

            # Draw supply center as small dot above province
            rl.DrawCircle(int(x), int(y - 25), SC_SIZE // 2, color)
            rl.DrawCircleLines(int(x), int(y - 25), SC_SIZE // 2, (0, 0, 0, 255))

    def _draw_labels(self, font_size=None):
        """Draw province name labels using optimized label coordinates."""
        if font_size is None:
            font_size = LABEL_FONT_SIZE

        for name, (x, y) in LABEL_COORDS.items():
            if x == 0 and y == 0:
                continue
            label = name[:3]

            if self.font:
                # Use custom font for crisp text
                text_size = rl.MeasureTextEx(self.font, label.encode(), font_size, 1)
                pos = ffi.new('Vector2 *', [x - text_size.x / 2, y - text_size.y / 2])[0]
                rl.DrawTextEx(self.font, label.encode(), pos, font_size, 1, (20, 20, 20, 255))
            else:
                # Fallback to default font
                text_width = rl.MeasureText(label.encode(), font_size)
                rl.DrawText(label.encode(), int(x - text_width / 2), int(y - font_size / 2), font_size, (20, 20, 20, 255))

    def _draw_units(self, game_state):
        """Draw all units on the map."""
        for power_id, power_data in enumerate(game_state['powers']):
            color = get_power_unit_color(power_id)
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

    def _draw_textured_unit(self, texture, x, y, color):
        """Draw a unit with texture, shadow, and colored background."""
        w, h = texture.width, texture.height
        # Shadow
        rl.DrawRectangleRounded(
            ffi.new('Rectangle *', [x - w/2 + 2, y - h/2 + 2, w, h])[0],
            0.3, 4, (0, 0, 0, 100)
        )
        # Colored background
        rl.DrawRectangleRounded(
            ffi.new('Rectangle *', [x - w/2, y - h/2, w, h])[0],
            0.3, 4, color
        )
        # Icon
        rl.DrawTexture(texture, int(x - w/2), int(y - h/2), colors.WHITE)

    def _draw_army(self, x, y, color):
        """Draw army symbol (tank icon or fallback circle)."""
        if self.army_texture:
            self._draw_textured_unit(self.army_texture, x, y, color)
        else:
            rl.DrawCircle(int(x + 2), int(y + 2), UNIT_RADIUS, (0, 0, 0, 100))
            rl.DrawCircle(int(x), int(y), UNIT_RADIUS, color)
            rl.DrawCircleLines(int(x), int(y), UNIT_RADIUS, (0, 0, 0, 255))

    def _draw_fleet(self, x, y, color):
        """Draw fleet symbol (ship icon or fallback diamond)."""
        if self.fleet_texture:
            self._draw_textured_unit(self.fleet_texture, x, y, color)
        else:
            size = FLEET_SIZE
            self._draw_diamond(x + 2, y + 2, size, (0, 0, 0, 100))
            self._draw_diamond(x, y, size, color)
            self._draw_diamond_lines(x, y, size, (0, 0, 0, 255))

    def _draw_diamond(self, x, y, size, color):
        """Draw filled diamond shape."""
        points = [
            (x, y - size),      # Top
            (x + size, y),      # Right
            (x, y + size),      # Bottom
            (x - size, y),      # Left
        ]
        # Draw as two triangles (using ffi for Vector2)
        def vec2(x, y):
            return ffi.new('Vector2 *', [x, y])[0]
        rl.DrawTriangle(
            vec2(points[0][0], points[0][1]),
            vec2(points[1][0], points[1][1]),
            vec2(points[2][0], points[2][1]),
            color
        )
        rl.DrawTriangle(
            vec2(points[0][0], points[0][1]),
            vec2(points[2][0], points[2][1]),
            vec2(points[3][0], points[3][1]),
            color
        )

    def _draw_diamond_lines(self, x, y, size, color):
        """Draw diamond outline."""
        rl.DrawLine(int(x), int(y - size), int(x + size), int(y), color)
        rl.DrawLine(int(x + size), int(y), int(x), int(y + size), color)
        rl.DrawLine(int(x), int(y + size), int(x - size), int(y), color)
        rl.DrawLine(int(x - size), int(y), int(x), int(y - size), color)

    def _draw_filled_polygon(self, polygon, color):
        """Draw a filled polygon using ear-clipping triangulation."""
        if len(polygon) < 3:
            return

        def vec2(x, y):
            return ffi.new('Vector2 *', [float(x), float(y)])[0]

        # Convert to mutable list
        verts = list(polygon)

        # Ensure counter-clockwise winding
        def signed_area(pts):
            area = 0
            n = len(pts)
            for i in range(n):
                j = (i + 1) % n
                area += pts[i][0] * pts[j][1]
                area -= pts[j][0] * pts[i][1]
            return area / 2

        if signed_area(verts) > 0:
            verts = verts[::-1]

        # Check if point is inside triangle
        def point_in_triangle(px, py, ax, ay, bx, by, cx, cy):
            def sign(p1x, p1y, p2x, p2y, p3x, p3y):
                return (p1x - p3x) * (p2y - p3y) - (p2x - p3x) * (p1y - p3y)
            d1 = sign(px, py, ax, ay, bx, by)
            d2 = sign(px, py, bx, by, cx, cy)
            d3 = sign(px, py, cx, cy, ax, ay)
            has_neg = (d1 < 0) or (d2 < 0) or (d3 < 0)
            has_pos = (d1 > 0) or (d2 > 0) or (d3 > 0)
            return not (has_neg and has_pos)

        # Check if vertex is an ear
        def is_ear(i, pts):
            n = len(pts)
            prev_i = (i - 1) % n
            next_i = (i + 1) % n
            a, b, c = pts[prev_i], pts[i], pts[next_i]
            # Check if convex (cross product)
            cross = (b[0] - a[0]) * (c[1] - a[1]) - (b[1] - a[1]) * (c[0] - a[0])
            if cross >= 0:
                return False
            # Check no other vertex inside triangle
            for j in range(n):
                if j in (prev_i, i, next_i):
                    continue
                if point_in_triangle(pts[j][0], pts[j][1], a[0], a[1], b[0], b[1], c[0], c[1]):
                    return False
            return True

        # Ear clipping
        triangles = []
        indices = list(range(len(verts)))
        max_iterations = len(verts) * 3  # Safety limit

        while len(indices) > 3 and max_iterations > 0:
            max_iterations -= 1
            found_ear = False
            for i in range(len(indices)):
                pts = [verts[j] for j in indices]
                if is_ear(i, pts):
                    prev_i = (i - 1) % len(indices)
                    next_i = (i + 1) % len(indices)
                    triangles.append((indices[prev_i], indices[i], indices[next_i]))
                    indices.pop(i)
                    found_ear = True
                    break
            if not found_ear:
                break

        # Add final triangle
        if len(indices) == 3:
            triangles.append((indices[0], indices[1], indices[2]))

        # Draw all triangles
        for i0, i1, i2 in triangles:
            rl.DrawTriangle(
                vec2(verts[i0][0], verts[i0][1]),
                vec2(verts[i1][0], verts[i1][1]),
                vec2(verts[i2][0], verts[i2][1]),
                color
            )

    def _draw_polygon_outline(self, polygon, color, thickness=1):
        """Draw polygon outline with given color and thickness."""
        if len(polygon) < 2:
            return
        for i in range(len(polygon)):
            p1 = polygon[i]
            p2 = polygon[(i + 1) % len(polygon)]
            if thickness > 1:
                rl.DrawLineEx(
                    ffi.new('Vector2 *', [float(p1[0]), float(p1[1])])[0],
                    ffi.new('Vector2 *', [float(p2[0]), float(p2[1])])[0],
                    float(thickness),
                    color
                )
            else:
                rl.DrawLine(int(p1[0]), int(p1[1]), int(p2[0]), int(p2[1]), color)

    def _draw_orders(self, game_state):
        """Draw order visualizations (arrows, support lines, etc.)."""
        # TODO: Integrate with OrderVisualizer when order data is exposed
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
