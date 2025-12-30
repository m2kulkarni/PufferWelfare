#!/usr/bin/env python3
"""
Generate a GIF demonstration of Welfare Diplomacy moves.

This script creates a headless visualization using PIL (no display required)
and saves the result as a GIF.
"""

import os
import sys

# Add project to path
sys.path.insert(0, os.path.dirname(os.path.dirname(os.path.abspath(__file__))))

from PIL import Image, ImageDraw, ImageFont
import imageio
import numpy as np

from pufferlib.ocean.diplomacy import binding
from pufferlib.ocean.diplomacy.render.map_data import (
    MAP_WIDTH, MAP_HEIGHT, UNIT_RADIUS, PROVINCE_COORDS,
    POWER_COLORS, POWER_NAMES, PHASE_NAMES, LOC_INDEX_TO_NAME,
    get_coords, get_power_color, get_location_name,
)


class HeadlessRenderer:
    """Simple PIL-based renderer for headless GIF generation."""

    def __init__(self, width=800, height=700):
        self.width = width
        self.height = height
        self.scale = min(width / MAP_WIDTH, height / MAP_HEIGHT) * 0.95
        self.offset_x = (width - MAP_WIDTH * self.scale) / 2
        self.offset_y = (height - MAP_HEIGHT * self.scale) / 2

    def world_to_screen(self, x, y):
        """Convert world coordinates to screen coordinates."""
        return (
            int(x * self.scale + self.offset_x),
            int(y * self.scale + self.offset_y)
        )

    def render_frame(self, game_state, title="", orders=None):
        """Render a single frame.

        Args:
            game_state: Game state dict from query_game_state()
            title: Title text to display
            orders: Optional dict of power_id -> [order_strings]

        Returns:
            PIL Image
        """
        # Create image with sea blue background
        img = Image.new('RGB', (self.width, self.height), (64, 128, 192))
        draw = ImageDraw.Draw(img)

        # Draw land areas (simplified - just colored rectangles for key areas)
        # This is a very simplified representation
        land_color = (200, 180, 160)

        # Draw supply center markers and ownership
        ownership = {}
        for power_id, power_data in enumerate(game_state['powers']):
            for center_idx in power_data['centers']:
                center_name = get_location_name(center_idx)
                ownership[center_name] = power_id

        # Draw province dots for context
        for name, (wx, wy) in PROVINCE_COORDS.items():
            if "/" in name:  # Skip coast variants
                continue
            sx, sy = self.world_to_screen(wx, wy)
            # Draw small dot for province
            draw.ellipse([sx-3, sy-3, sx+3, sy+3], fill=(150, 150, 150))

        # Draw units
        for power_id, power_data in enumerate(game_state['powers']):
            color = get_power_color(power_id)
            for unit in power_data['units']:
                loc_idx = unit['location']
                loc_name = get_location_name(loc_idx)
                wx, wy = get_coords(loc_name)
                if wx == 0 and wy == 0:
                    continue

                sx, sy = self.world_to_screen(wx, wy)
                radius = int(UNIT_RADIUS * self.scale)

                if unit['type'] == 1:  # Army
                    # Draw circle
                    draw.ellipse(
                        [sx - radius, sy - radius, sx + radius, sy + radius],
                        fill=color, outline=(0, 0, 0), width=2
                    )
                else:  # Fleet
                    # Draw diamond
                    points = [
                        (sx, sy - radius),
                        (sx + radius, sy),
                        (sx, sy + radius),
                        (sx - radius, sy),
                    ]
                    draw.polygon(points, fill=color, outline=(0, 0, 0), width=2)

                # Draw location label
                draw.text((sx - 10, sy + radius + 2), loc_name[:3], fill=(0, 0, 0))

        # Draw orders as arrows
        if orders:
            for power_id, order_list in orders.items():
                color = get_power_color(power_id)
                for order_str in order_list:
                    self._draw_order(draw, order_str, color)

        # Draw HUD
        year = game_state['year']
        phase = game_state['phase']
        phase_name = PHASE_NAMES.get(phase, f"Phase {phase}")

        # Title
        draw.rectangle([0, 0, self.width, 30], fill=(0, 0, 0, 200))
        draw.text((10, 5), f"{title} - {phase_name} {year}", fill=(0, 200, 200))

        # Power stats
        y_offset = 35
        for power_id, power_data in enumerate(game_state['powers']):
            if power_data['num_units'] == 0 and power_data['num_centers'] == 0:
                continue
            color = get_power_color(power_id)
            name = POWER_NAMES[power_id][:3]
            centers = power_data['num_centers']
            units = power_data['num_units']

            draw.rectangle([5, y_offset, 15, y_offset + 10], fill=color)
            draw.text((20, y_offset - 2), f"{name}: C{centers} U{units}", fill=(255, 255, 255))
            y_offset += 15

        return img

    def _draw_order(self, draw, order_str, color):
        """Draw an order as an arrow."""
        parts = order_str.split()
        if len(parts) < 3:
            return

        from_loc = parts[1]

        if parts[2] == '-' and len(parts) >= 4:
            # Move order
            to_loc = parts[3]
            self._draw_arrow(draw, from_loc, to_loc, color)
        elif parts[2] == 'S' and len(parts) >= 4:
            # Support order - draw dashed line
            target_loc = parts[-1] if '-' not in parts else parts[parts.index('-') + 1]
            self._draw_dashed_line(draw, from_loc, target_loc, color)

    def _draw_arrow(self, draw, from_loc, to_loc, color):
        """Draw an arrow from one location to another."""
        wx1, wy1 = get_coords(from_loc)
        wx2, wy2 = get_coords(to_loc)

        if (wx1, wy1) == (0, 0) or (wx2, wy2) == (0, 0):
            return

        sx1, sy1 = self.world_to_screen(wx1, wy1)
        sx2, sy2 = self.world_to_screen(wx2, wy2)

        # Shorten arrow
        import math
        dx, dy = sx2 - sx1, sy2 - sy1
        length = math.sqrt(dx * dx + dy * dy)
        if length > 0:
            r = int(UNIT_RADIUS * self.scale)
            sx1 = int(sx1 + r * dx / length)
            sy1 = int(sy1 + r * dy / length)
            sx2 = int(sx2 - (r + 5) * dx / length)
            sy2 = int(sy2 - (r + 5) * dy / length)

        # Draw line
        draw.line([(sx1, sy1), (sx2, sy2)], fill=color, width=3)

        # Draw arrowhead
        if length > 0:
            dx, dy = dx / length, dy / length
            px, py = -dy, dx
            size = 8
            points = [
                (sx2, sy2),
                (sx2 - size * dx + size * 0.5 * px, sy2 - size * dy + size * 0.5 * py),
                (sx2 - size * dx - size * 0.5 * px, sy2 - size * dy - size * 0.5 * py),
            ]
            draw.polygon([(int(x), int(y)) for x, y in points], fill=color)

    def _draw_dashed_line(self, draw, from_loc, to_loc, color):
        """Draw a dashed line (for support orders)."""
        wx1, wy1 = get_coords(from_loc)
        wx2, wy2 = get_coords(to_loc)

        if (wx1, wy1) == (0, 0) or (wx2, wy2) == (0, 0):
            return

        sx1, sy1 = self.world_to_screen(wx1, wy1)
        sx2, sy2 = self.world_to_screen(wx2, wy2)

        # Draw dashed
        import math
        dx, dy = sx2 - sx1, sy2 - sy1
        length = math.sqrt(dx * dx + dy * dy)
        if length == 0:
            return

        dx, dy = dx / length, dy / length
        dash_len = 8
        gap_len = 4

        pos = 0
        while pos < length:
            end = min(pos + dash_len, length)
            x1, y1 = sx1 + dx * pos, sy1 + dy * pos
            x2, y2 = sx1 + dx * end, sy1 + dy * end
            draw.line([(int(x1), int(y1)), (int(x2), int(y2))], fill=color, width=2)
            pos += dash_len + gap_len


def create_demo_gif():
    """Create a demonstration GIF with sample moves."""

    # Initialize environment
    obs = np.zeros((7, 175), dtype=np.float32)
    actions = np.zeros((7,), dtype=np.int32)
    rewards = np.zeros((7,), dtype=np.float32)
    terminals = np.zeros((7,), dtype=np.uint8)
    truncations = np.zeros((7,), dtype=np.uint8)

    env_handle = binding.env_init(obs, actions, rewards, terminals, truncations, 42)
    binding.env_reset(env_handle, 42)

    renderer = HeadlessRenderer(width=800, height=700)
    frames = []

    # Frame 1: Initial state
    game_state = binding.query_game_state(env_handle)
    frame = renderer.render_frame(game_state, "Initial Setup")
    frames.append(np.array(frame))

    # Set up a simple scenario: Test 6.A.1 - Basic movement
    # Clear all units first
    for p in range(7):
        binding.game_clear_units(env_handle, p)

    # Set Austria (0) with one army in Vienna
    binding.game_set_units(env_handle, 0, [('A', 'VIE')])
    binding.game_set_centers(env_handle, 0, ['VIE', 'BUD', 'TRI'])

    # Set Italy (4) with army in Venice
    binding.game_set_units(env_handle, 4, [('A', 'VEN')])
    binding.game_set_centers(env_handle, 4, ['VEN', 'ROM', 'NAP'])

    # Set Germany (3) with fleet in Kiel
    binding.game_set_units(env_handle, 3, [('F', 'KIE')])
    binding.game_set_centers(env_handle, 3, ['KIE', 'BER', 'MUN'])

    # Set France (2) with army in Paris
    binding.game_set_units(env_handle, 2, [('A', 'PAR')])
    binding.game_set_centers(env_handle, 2, ['PAR', 'BRE', 'MAR'])

    # Set England (1) with fleet in London
    binding.game_set_units(env_handle, 1, [('F', 'LON')])
    binding.game_set_centers(env_handle, 1, ['LON', 'LVP', 'EDI'])

    # Set Russia (5) with army in Moscow
    binding.game_set_units(env_handle, 5, [('A', 'MOS')])
    binding.game_set_centers(env_handle, 5, ['MOS', 'SEV', 'STP', 'WAR'])

    # Set Turkey (6) with army in Constantinople
    binding.game_set_units(env_handle, 6, [('A', 'CON')])
    binding.game_set_centers(env_handle, 6, ['CON', 'ANK', 'SMY'])

    # Frame 2: Show setup
    game_state = binding.query_game_state(env_handle)
    frame = renderer.render_frame(game_state, "Spring 1901 - Setup")
    frames.append(np.array(frame))

    # Define orders
    orders = {
        0: ['A VIE - TRI'],       # Austria: Vienna to Trieste
        1: ['F LON - NTH'],       # England: London to North Sea
        2: ['A PAR - BUR'],       # France: Paris to Burgundy
        3: ['F KIE - DEN'],       # Germany: Kiel to Denmark
        4: ['A VEN - TYR'],       # Italy: Venice to Tyrolia
        5: ['A MOS - UKR'],       # Russia: Moscow to Ukraine
        6: ['A CON - BUL'],       # Turkey: Constantinople to Bulgaria
    }

    # Frame 3: Show orders as arrows
    frame = renderer.render_frame(game_state, "Spring 1901 - Orders", orders)
    frames.append(np.array(frame))

    # Submit orders to engine
    for power_id, order_list in orders.items():
        binding.game_submit_orders(env_handle, power_id, order_list)

    # Process orders
    binding.env_step(env_handle)

    # Frame 4: Show results
    game_state = binding.query_game_state(env_handle)
    frame = renderer.render_frame(game_state, "Spring 1901 - Results")
    frames.append(np.array(frame))

    # Duplicate last frame for pause
    frames.append(np.array(frame))
    frames.append(np.array(frame))

    # Clean up
    binding.env_close(env_handle)

    # Save as GIF
    output_path = os.path.join(os.path.dirname(__file__), '..', 'docs', 'diplomacy_demo.gif')
    imageio.mimsave(output_path, frames, duration=1.5, loop=0)
    print(f"GIF saved to: {output_path}")

    # Also save individual frames as PNG for inspection
    for i, frame in enumerate(frames[:4]):
        frame_path = os.path.join(os.path.dirname(__file__), '..', 'docs', f'frame_{i}.png')
        Image.fromarray(frame).save(frame_path)
        print(f"Frame {i} saved to: {frame_path}")

    return output_path


if __name__ == '__main__':
    create_demo_gif()
