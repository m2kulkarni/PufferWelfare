"""
Order visualization for Welfare Diplomacy.

Draws order visualizations:
- Move arrows (line + arrowhead)
- Support hold (dashed line + circle)
- Support move (Bezier curve + arrowhead)
- Convoy (triangle symbol + dashed lines)
- Hold (concentric circles)
- Build (starburst), Disband (X)
"""

import math

try:
    from raylib import rl
    import pyray
    RAYLIB_AVAILABLE = True
except ImportError:
    RAYLIB_AVAILABLE = False

from .map_data import (
    UNIT_RADIUS, ARROW_HEAD_SIZE, LINE_WIDTH,
    get_coords, get_coords_by_index, get_power_color, get_location_name,
)


class OrderVisualizer:
    """
    Draws order visualizations on the map.
    """

    def __init__(self):
        """Initialize order visualizer."""
        pass

    def draw_move(self, from_loc, to_loc, color, is_retreat=False, via_convoy=False):
        """Draw move order as arrow from source to destination.

        Args:
            from_loc: Source location name (e.g., "PAR")
            to_loc: Destination location name (e.g., "BUR")
            color: RGB tuple for the arrow color
            is_retreat: If True, draw with thinner line
            via_convoy: If True, draw dashed line
        """
        x1, y1 = get_coords(from_loc)
        x2, y2 = get_coords(to_loc)

        if (x1, y1) == (0, 0) or (x2, y2) == (0, 0):
            return

        # Shorten arrow to not overlap with unit
        dx, dy = x2 - x1, y2 - y1
        length = math.sqrt(dx * dx + dy * dy)
        if length > 0:
            # Shorten both ends
            x1 = x1 + (UNIT_RADIUS + 2) / length * dx
            y1 = y1 + (UNIT_RADIUS + 2) / length * dy
            x2 = x2 - (UNIT_RADIUS + ARROW_HEAD_SIZE) / length * dx
            y2 = y2 - (UNIT_RADIUS + ARROW_HEAD_SIZE) / length * dy

        # Shadow
        shadow_color = (0, 0, 0, 100)
        line_width = LINE_WIDTH - 1 if is_retreat else LINE_WIDTH
        rl.DrawLineEx(
            pyray.Vector2(x1 + 2, y1 + 2),
            pyray.Vector2(x2 + 2, y2 + 2),
            line_width + 2,
            shadow_color
        )

        # Main line
        if via_convoy:
            self._draw_dashed_line(x1, y1, x2, y2, color, line_width)
        else:
            rl.DrawLineEx(
                pyray.Vector2(x1, y1),
                pyray.Vector2(x2, y2),
                line_width,
                color
            )

        # Arrow head
        self._draw_arrowhead(x1, y1, x2, y2, color)

    def draw_hold(self, loc, color):
        """Draw hold order as concentric circles.

        Args:
            loc: Location name (e.g., "PAR")
            color: RGB tuple for the circle color
        """
        x, y = get_coords(loc)
        if (x, y) == (0, 0):
            return

        # Draw two concentric circles
        rl.DrawCircleLines(int(x), int(y), UNIT_RADIUS + 4, color)
        rl.DrawCircleLines(int(x), int(y), UNIT_RADIUS + 7, color)

    def draw_support_hold(self, from_loc, target_loc, color):
        """Draw support hold as dashed line with circle at target.

        Args:
            from_loc: Supporting unit location (e.g., "BUR")
            target_loc: Unit being supported location (e.g., "PAR")
            color: RGB tuple for the line color
        """
        x1, y1 = get_coords(from_loc)
        x2, y2 = get_coords(target_loc)

        if (x1, y1) == (0, 0) or (x2, y2) == (0, 0):
            return

        # Shorten to not overlap with units
        dx, dy = x2 - x1, y2 - y1
        length = math.sqrt(dx * dx + dy * dy)
        if length > 0:
            x1 = x1 + (UNIT_RADIUS + 2) / length * dx
            y1 = y1 + (UNIT_RADIUS + 2) / length * dy
            x2 = x2 - (UNIT_RADIUS + 5) / length * dx
            y2 = y2 - (UNIT_RADIUS + 5) / length * dy

        # Draw dashed line
        self._draw_dashed_line(x1, y1, x2, y2, color, LINE_WIDTH - 1)

        # Draw circle at target
        tx, ty = get_coords(target_loc)
        rl.DrawCircleLines(int(tx), int(ty), UNIT_RADIUS + 5, color)

    def draw_support_move(self, from_loc, supported_from, supported_to, color):
        """Draw support move as curved line (Bezier) from supporter to destination.

        Args:
            from_loc: Supporting unit location (e.g., "BUR")
            supported_from: Supported unit's current location (e.g., "PAR")
            supported_to: Supported unit's destination (e.g., "MAR")
            color: RGB tuple for the line color
        """
        x1, y1 = get_coords(from_loc)
        x2, y2 = get_coords(supported_from)
        x3, y3 = get_coords(supported_to)

        if (x1, y1) == (0, 0) or (x3, y3) == (0, 0):
            return

        # Draw bezier curve from supporter through supported unit to destination
        # Control point is the supported unit's position
        rl.DrawLineBezier(
            pyray.Vector2(x1, y1),
            pyray.Vector2(x3, y3),
            LINE_WIDTH - 1,
            color
        )

        # Draw arrowhead at destination
        self._draw_arrowhead(x2, y2, x3, y3, color)

    def draw_convoy(self, fleet_loc, army_from, army_to, color):
        """Draw convoy order as triangle at army's origin.

        Args:
            fleet_loc: Convoying fleet location (e.g., "ENG")
            army_from: Army's origin location (e.g., "LON")
            army_to: Army's destination (e.g., "BRE")
            color: RGB tuple for the line color
        """
        x1, y1 = get_coords(fleet_loc)
        x2, y2 = get_coords(army_from)
        x3, y3 = get_coords(army_to)

        if (x1, y1) == (0, 0) or (x2, y2) == (0, 0):
            return

        # Draw dashed line from fleet to army origin
        self._draw_dashed_line(x1, y1, x2, y2, color, LINE_WIDTH - 1)

        # Draw convoy triangle at army origin
        self._draw_convoy_triangle(x2, y2, color)

        # Draw dashed line from triangle to destination
        if (x3, y3) != (0, 0):
            self._draw_dashed_line(x2, y2, x3, y3, color, LINE_WIDTH - 1)

    def draw_build(self, loc, unit_type, color):
        """Draw build order as unit with starburst.

        Args:
            loc: Build location (e.g., "PAR")
            unit_type: 1 for Army, 2 for Fleet
            color: RGB tuple for the starburst color
        """
        x, y = get_coords(loc)
        if (x, y) == (0, 0):
            return

        # Draw starburst around position
        for i in range(8):
            angle = i * math.pi / 4
            x1 = x + (UNIT_RADIUS + 8) * math.cos(angle)
            y1 = y + (UNIT_RADIUS + 8) * math.sin(angle)
            x2 = x + (UNIT_RADIUS + 15) * math.cos(angle)
            y2 = y + (UNIT_RADIUS + 15) * math.sin(angle)
            rl.DrawLine(int(x1), int(y1), int(x2), int(y2), color)

    def draw_disband(self, loc, color=None):
        """Draw disband order as X over unit.

        Args:
            loc: Unit location (e.g., "PAR")
            color: RGB tuple for X color (defaults to red)
        """
        x, y = get_coords(loc)
        if (x, y) == (0, 0):
            return

        if color is None:
            color = (255, 0, 0, 255)

        size = UNIT_RADIUS + 5
        rl.DrawLineEx(
            pyray.Vector2(x - size, y - size),
            pyray.Vector2(x + size, y + size),
            3,
            color
        )
        rl.DrawLineEx(
            pyray.Vector2(x - size, y + size),
            pyray.Vector2(x + size, y - size),
            3,
            color
        )

    def draw_retreat(self, from_loc, to_loc, color):
        """Draw retreat order (same as move but thinner).

        Args:
            from_loc: Unit's current location
            to_loc: Retreat destination
            color: RGB tuple for the arrow color
        """
        self.draw_move(from_loc, to_loc, color, is_retreat=True)

    def _draw_arrowhead(self, x1, y1, x2, y2, color):
        """Draw arrowhead at the end of a line.

        Args:
            x1, y1: Start point of the line
            x2, y2: End point of the line (arrowhead tip)
            color: RGB tuple for the arrowhead color
        """
        dx, dy = x2 - x1, y2 - y1
        length = math.sqrt(dx * dx + dy * dy)
        if length == 0:
            return

        # Normalize direction
        dx, dy = dx / length, dy / length

        # Perpendicular direction
        px, py = -dy, dx

        # Arrowhead points
        size = ARROW_HEAD_SIZE
        ax1 = x2 - size * dx + size * 0.5 * px
        ay1 = y2 - size * dy + size * 0.5 * py
        ax2 = x2 - size * dx - size * 0.5 * px
        ay2 = y2 - size * dy - size * 0.5 * py

        # Draw filled triangle
        rl.DrawTriangle(
            pyray.Vector2(x2, y2),
            pyray.Vector2(ax2, ay2),
            pyray.Vector2(ax1, ay1),
            color
        )

    def _draw_dashed_line(self, x1, y1, x2, y2, color, width=2):
        """Draw a dashed line from (x1, y1) to (x2, y2).

        Args:
            x1, y1: Start point
            x2, y2: End point
            color: RGB tuple for the line color
            width: Line width
        """
        dx, dy = x2 - x1, y2 - y1
        length = math.sqrt(dx * dx + dy * dy)
        if length == 0:
            return

        dx, dy = dx / length, dy / length

        dash_length = 8
        gap_length = 4
        segment_length = dash_length + gap_length

        num_segments = int(length / segment_length)
        for i in range(num_segments + 1):
            start_dist = i * segment_length
            end_dist = min(start_dist + dash_length, length)

            sx = x1 + dx * start_dist
            sy = y1 + dy * start_dist
            ex = x1 + dx * end_dist
            ey = y1 + dy * end_dist

            rl.DrawLineEx(
                pyray.Vector2(sx, sy),
                pyray.Vector2(ex, ey),
                width,
                color
            )

    def _draw_convoy_triangle(self, x, y, color):
        """Draw convoy triangle symbol at a position.

        Args:
            x, y: Center position
            color: RGB tuple for the triangle color
        """
        size = 10
        # Equilateral triangle pointing up
        points = [
            (x, y - size),           # Top
            (x + size * 0.866, y + size * 0.5),  # Bottom right
            (x - size * 0.866, y + size * 0.5),  # Bottom left
        ]

        rl.DrawTriangle(
            pyray.Vector2(points[0][0], points[0][1]),
            pyray.Vector2(points[2][0], points[2][1]),
            pyray.Vector2(points[1][0], points[1][1]),
            color
        )
        # Outline
        rl.DrawTriangleLines(
            pyray.Vector2(points[0][0], points[0][1]),
            pyray.Vector2(points[2][0], points[2][1]),
            pyray.Vector2(points[1][0], points[1][1]),
            (0, 0, 0, 255)
        )


def draw_order_result(loc, result, success_color=(0, 255, 0, 200), fail_color=(255, 0, 0, 200)):
    """Draw order result indicator (success/fail).

    Args:
        loc: Location name
        result: Result code (1 = success, 2+ = failure)
        success_color: Color for successful orders
        fail_color: Color for failed orders
    """
    x, y = get_coords(loc)
    if (x, y) == (0, 0):
        return

    color = success_color if result == 1 else fail_color
    rl.DrawCircleLines(int(x), int(y), UNIT_RADIUS + 3, color)
