"""Extract province paths from SvgStandard.js and create a proper SVG map."""
import re

# Province colors for initial neutral state
COLORS = {
    'water': '#c5dfea',     # Light blue for sea
    'neutral': '#d3d3d3',   # Light gray for neutral land
    'impassable': '#353433', # Dark for Switzerland
}

# Water provinces
WATER_PROVINCES = {
    '_nat', '_nrg', '_bar', '_bot', '_bal', '_ska', '_hel', '_nth',
    '_eng', '_iri', '_mid', '_wes', '_gol', '_tyn', '_adr', '_ion',
    '_aeg', '_eas', '_bla'
}

def extract_paths(js_file):
    """Extract SVG path data from SvgStandard.js"""
    with open(js_file, 'r') as f:
        content = f.read()

    # Find all path elements with d= and id= attributes
    # Pattern: <path className={...} d="..." id="..."/>
    pattern = r'<path\s+className=\{[^}]+\}\s+d="([^"]+)"\s+id="([^"]+)"[^/]*/>'
    matches = re.findall(pattern, content)

    return matches

def create_svg(paths, output_file):
    """Create SVG file from extracted paths."""
    # Keep original viewBox and transform to match unit coordinates from jdipNS:PROVINCE_DATA
    svg_header = '''<?xml version="1.0" encoding="UTF-8"?>
<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 1835 1360" width="1835" height="1360">
  <defs>
    <style>
      .water { fill: #c5dfea; stroke: #000000; stroke-width: 1; }
      .land { fill: #f5e6d3; stroke: #000000; stroke-width: 1; }
      .impassable { fill: #353433; stroke: #000000; stroke-width: 1; }
    </style>
  </defs>
  <g id="MapLayer" transform="translate(-195 -170)">
'''

    svg_footer = '''  </g>
</svg>
'''

    with open(output_file, 'w') as f:
        f.write(svg_header)

        for path_d, path_id in paths:
            # Determine province type
            if path_id in WATER_PROVINCES or 'water' in path_id.lower():
                css_class = 'water'
            elif path_id == '_swi':
                css_class = 'impassable'
            else:
                css_class = 'land'

            f.write(f'    <path class="{css_class}" d="{path_d}" id="{path_id}"/>\n')

        f.write(svg_footer)

    print(f"Created SVG with {len(paths)} provinces")
    return output_file

def main():
    js_file = '/scratch/mmk9418/projects/welfare-diplomacy/diplomacy/web/src/gui/maps/standard/SvgStandard.js'
    output_svg = '/scratch/mmk9418/projects/PufferWelfare/pufferlib/ocean/diplomacy/render/assets/map_provinces.svg'
    output_png = '/scratch/mmk9418/projects/PufferWelfare/pufferlib/ocean/diplomacy/render/assets/map_bg.png'

    print(f"Extracting paths from {js_file}...")
    paths = extract_paths(js_file)
    print(f"Found {len(paths)} province paths")

    print(f"Creating SVG at {output_svg}...")
    create_svg(paths, output_svg)

    # Convert to PNG
    print(f"Converting to PNG at {output_png}...")
    try:
        import cairosvg
        cairosvg.svg2png(url=output_svg, write_to=output_png, output_width=1835, output_height=1360)
        print("Done!")
    except Exception as e:
        print(f"Error converting to PNG: {e}")
        print("You can manually convert using: cairosvg map_provinces.svg -o map_bg.png")

if __name__ == '__main__':
    main()
