"""
Script to parse the standard.map file and generate C code for init_standard_map()

Usage: python generate_map_data.py <path_to_standard.map>

This script parses a Diplomacy map file and outputs C code that can be
copied into diplomacy_map.c to initialize the map data structures.
"""

import re
import sys
import os

# Neutral supply centers in standard Diplomacy
NEUTRAL_SUPPLY_CENTERS = {'BEL', 'BUL', 'DEN', 'GRE', 'HOL', 'NWY', 'POR', 'RUM', 'SER', 'SPA', 'SWE', 'TUN'}

def main():
    if len(sys.argv) < 2:
        # Default path for convenience during development
        script_dir = os.path.dirname(os.path.abspath(__file__))
        map_file = os.path.join(script_dir, "../../../../welfare-diplomacy/diplomacy/maps/standard.map")
        if not os.path.exists(map_file):
            print(f"Usage: {sys.argv[0]} <path_to_standard.map>", file=sys.stderr)
            sys.exit(1)
    else:
        map_file = sys.argv[1]

    locations = []
    loc_types = {}
    adjacencies = {}
    home_centers = {}

    with open(map_file, 'r') as f:
        content = f.read()

    # Parse initial power setup - match each line separately
    # Format: "AUSTRIA     (AUSTRIAN)     BUD TRI VIE"
    power_pattern = r'^([A-Z]+)\s+\([A-Z]+\)\s+([A-Z ]+)$'
    for match in re.finditer(power_pattern, content, re.MULTILINE):
        power = match.group(1)
        centers = match.group(2).split()
        home_centers[power] = centers

    # Parse location types and adjacencies
    adjacency_pattern = r'(\w+)\s+(\w+(?:/\w+)?)\s+ABUTS\s+(.+)'
    for match in re.finditer(adjacency_pattern, content):
        loc_type = match.group(1)
        location = match.group(2).upper()
        abuts_str = match.group(3)

        # Get canonical 3-letter code (strip coast suffixes)
        if '/' in location or '(' in location:
            loc = location.split('/')[0].split('(')[0]
        else:
            loc = location

        if loc not in loc_types:
            loc_types[loc] = loc_type
            locations.append(loc)

        # Parse adjacencies (strip coast suffixes)
        adj_list = [adj.upper().split('/')[0].split('(')[0] for adj in abuts_str.split()]
        adjacencies[loc] = adj_list

    # Sort locations alphabetically for consistency
    locations.sort()

    # Map location types to enum
    type_map = {
        'LAND': 'LOC_LAND',
        'COAST': 'LOC_COAST',
        'WATER': 'LOC_WATER',
        'PORT': 'LOC_PORT'
    }

    # Generate C code
    print(f"// Total locations: {len(locations)}")
    print()
    print("// Location initialization")
    print(f"map->num_locations = {len(locations)};")
    print()

    for i, loc in enumerate(locations):
        loc_type_enum = type_map.get(loc_types.get(loc, 'COAST'), 'LOC_COAST')
        print(f"// {i}: {loc}")
        print(f'strcpy(map->locations[{i}].name, "{loc}");')
        print(f'map->locations[{i}].type = {loc_type_enum};')

        # Check if it's a supply center (home center OR neutral)
        is_home = any(loc in centers for centers in home_centers.values())
        is_neutral = loc in NEUTRAL_SUPPLY_CENTERS
        is_sc = is_home or is_neutral

        print(f'map->locations[{i}].has_supply_center = {1 if is_sc else 0};')
        print(f'map->locations[{i}].owner_power = -1;  // Neutral initially')

        # Adjacencies
        if loc in adjacencies:
            adj_indices = [locations.index(adj) for adj in adjacencies[loc] if adj in locations]
            print(f'map->locations[{i}].num_adjacent = {len(adj_indices)};')
            for j, adj_idx in enumerate(adj_indices):
                print(f'map->locations[{i}].adjacencies[{j}] = {adj_idx};')
        else:
            print(f'map->locations[{i}].num_adjacent = 0;')

        print()

    # Print home center assignments
    print("\n// Home center assignments")
    power_list = ['AUSTRIA', 'ENGLAND', 'FRANCE', 'GERMANY', 'ITALY', 'RUSSIA', 'TURKEY']
    for p_idx, power in enumerate(power_list):
        if power in home_centers:
            print(f"// {power}")
            centers = home_centers[power]
            for c_idx, center in enumerate(centers):
                if center in locations:
                    loc_idx = locations.index(center)
                    print(f'map->home_centers[{p_idx}][{c_idx}] = {loc_idx};  // {center}')
                    print(f'map->locations[{loc_idx}].is_home_center = {p_idx};')
                    print(f'map->locations[{loc_idx}].has_supply_center = 1;')
            print(f'map->num_homes[{p_idx}] = {len(centers)};')
            print()

    # Print neutral supply center assignments
    print("\n// Neutral supply centers")
    for sc in sorted(NEUTRAL_SUPPLY_CENTERS):
        if sc in locations:
            idx = locations.index(sc)
            print(f'map->locations[{idx}].has_supply_center = 1;  // {sc}')

    print("\n// Location name to index mapping (for reference):")
    for i, loc in enumerate(locations):
        print(f"// {i:2d}: {loc}")


if __name__ == "__main__":
    main()
