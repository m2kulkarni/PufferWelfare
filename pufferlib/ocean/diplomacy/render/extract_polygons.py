"""Extract province polygons from SVG paths for Raylib rendering."""
import re
import os


def parse_path_d(d):
    """Parse SVG path data and convert to polygon vertices.

    Handles M (moveto), L (lineto), C (cubic bezier), z (closepath).
    Curves are sampled to create line segments.
    """
    polygons = []
    current_polygon = []
    current_x, current_y = 0, 0

    # Tokenize the path data
    # Match command letters and numbers (including negatives and decimals)
    tokens = re.findall(r'[MLCZmlcz]|[-+]?\d*\.?\d+', d)

    i = 0
    while i < len(tokens):
        token = tokens[i]

        if token == 'M' or token == 'm':
            # Start new polygon on M
            if current_polygon:
                polygons.append(current_polygon)
                current_polygon = []

            if token == 'M':  # Absolute
                current_x = float(tokens[i + 1])
                current_y = float(tokens[i + 2])
            else:  # Relative
                current_x += float(tokens[i + 1])
                current_y += float(tokens[i + 2])

            current_polygon.append((current_x, current_y))
            i += 3

        elif token == 'L' or token == 'l':
            if token == 'L':  # Absolute
                current_x = float(tokens[i + 1])
                current_y = float(tokens[i + 2])
            else:  # Relative
                current_x += float(tokens[i + 1])
                current_y += float(tokens[i + 2])

            current_polygon.append((current_x, current_y))
            i += 3

        elif token == 'C' or token == 'c':
            # Cubic bezier curve - sample 4 points along the curve
            if token == 'C':  # Absolute
                x1, y1 = float(tokens[i + 1]), float(tokens[i + 2])
                x2, y2 = float(tokens[i + 3]), float(tokens[i + 4])
                x3, y3 = float(tokens[i + 5]), float(tokens[i + 6])
            else:  # Relative
                x1, y1 = current_x + float(tokens[i + 1]), current_y + float(tokens[i + 2])
                x2, y2 = current_x + float(tokens[i + 3]), current_y + float(tokens[i + 4])
                x3, y3 = current_x + float(tokens[i + 5]), current_y + float(tokens[i + 6])

            # Sample the cubic bezier curve
            p0 = (current_x, current_y)
            p1 = (x1, y1)
            p2 = (x2, y2)
            p3 = (x3, y3)

            # Sample at t = 0.25, 0.5, 0.75, 1.0
            for t in [0.25, 0.5, 0.75, 1.0]:
                bx = (1-t)**3 * p0[0] + 3*(1-t)**2*t * p1[0] + 3*(1-t)*t**2 * p2[0] + t**3 * p3[0]
                by = (1-t)**3 * p0[1] + 3*(1-t)**2*t * p1[1] + 3*(1-t)*t**2 * p2[1] + t**3 * p3[1]
                current_polygon.append((bx, by))

            current_x, current_y = x3, y3
            i += 7

        elif token == 'Z' or token == 'z':
            # Close path - add to polygons
            if current_polygon:
                polygons.append(current_polygon)
                current_polygon = []
            i += 1

        else:
            # Might be implicit lineto after M (numbers following M continue as L)
            try:
                x = float(token)
                y = float(tokens[i + 1])
                current_polygon.append((x, y))
                current_x, current_y = x, y
                i += 2
            except (ValueError, IndexError):
                i += 1

    # Don't forget any remaining polygon
    if current_polygon:
        polygons.append(current_polygon)

    return polygons


def extract_provinces_from_svg(svg_file):
    """Extract all province polygons from the SVG file."""
    with open(svg_file, 'r') as f:
        content = f.read()

    # Find all path elements with id and d attributes
    pattern = r'<path\s+class="[^"]*"\s+d="([^"]+)"\s+id="([^"]+)"'
    matches = re.findall(pattern, content)

    provinces = {}
    for path_d, path_id in matches:
        # Remove leading underscore from province name
        province_name = path_id[1:].upper() if path_id.startswith('_') else path_id.upper()

        # Parse path to polygons
        polygons = parse_path_d(path_d)

        if polygons:
            provinces[province_name] = polygons

    return provinces


def generate_python_module(provinces, output_file, x_offset=-195, y_offset=-170):
    """Generate a Python file with province polygon data.

    Args:
        provinces: Dict of province name to list of polygons
        output_file: Output Python file path
        x_offset, y_offset: Coordinate transform to apply (from SVG transform)
    """
    with open(output_file, 'w') as f:
        f.write('"""Province polygon data extracted from SVG paths."""\n\n')
        f.write('# Province polygons for Raylib rendering\n')
        f.write('# Each province maps to a list of polygons (for complex shapes with holes)\n')
        f.write('# Each polygon is a list of (x, y) vertices\n')
        f.write('# Coordinates have been transformed from SVG space to map space\n\n')
        f.write('PROVINCE_POLYGONS = {\n')

        for province_name in sorted(provinces.keys()):
            polygons = provinces[province_name]
            f.write(f'    "{province_name}": [\n')
            for polygon in polygons:
                # Simplify polygon - reduce points for better performance
                simplified = simplify_polygon(polygon, tolerance=3)
                if len(simplified) >= 3:
                    f.write('        [')
                    for i, (x, y) in enumerate(simplified):
                        # Apply coordinate transform
                        tx = x + x_offset
                        ty = y + y_offset
                        if i > 0:
                            f.write(', ')
                        f.write(f'({int(tx)}, {int(ty)})')
                    f.write('],\n')
            f.write('    ],\n')

        f.write('}\n')

    print(f"Generated {output_file} with {len(provinces)} provinces")


def simplify_polygon(points, tolerance=2):
    """Simplify polygon using Douglas-Peucker-like approach."""
    if len(points) < 3:
        return points

    # Simple point reduction - keep every Nth point
    # Also keep corners (high angle change)
    result = [points[0]]

    for i in range(1, len(points) - 1):
        p_prev = result[-1]
        p_curr = points[i]
        p_next = points[i + 1]

        # Distance from last kept point
        dist = ((p_curr[0] - p_prev[0])**2 + (p_curr[1] - p_prev[1])**2)**0.5

        # Keep if far enough from last point
        if dist > tolerance:
            result.append(p_curr)

    result.append(points[-1])

    # Ensure we have at least 3 points
    if len(result) < 3 and len(points) >= 3:
        return [points[0], points[len(points)//2], points[-1]]

    return result


def main():
    render_dir = os.path.dirname(os.path.abspath(__file__))
    svg_file = os.path.join(render_dir, "assets", "map_provinces.svg")
    output_file = os.path.join(render_dir, "province_polygons.py")

    print(f"Extracting provinces from {svg_file}...")
    provinces = extract_provinces_from_svg(svg_file)
    print(f"Found {len(provinces)} provinces")

    print(f"Generating {output_file}...")
    generate_python_module(provinces, output_file)


if __name__ == '__main__':
    main()
