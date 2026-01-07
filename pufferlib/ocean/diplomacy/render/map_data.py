"""
Map data for Welfare Diplomacy visualization.

Province coordinates extracted from standard.svg (jdipNS:PROVINCE_DATA).
Power colors from jdipNS:POWERCOLORS.
Location index mapping from diplomacy_map.c.
"""

# Map dimensions (from SVG viewbox: 0 0 1835 1360)
MAP_WIDTH = 1835
MAP_HEIGHT = 1360

# Unit rendering sizes
UNIT_RADIUS = 16
FLEET_SIZE = 12  # Smaller than circle to look balanced
ARROW_HEAD_SIZE = 10
LINE_WIDTH = 3
LABEL_FONT_SIZE = 20  # Crisp text at full resolution
SC_SIZE = 8

# Label coordinates - extracted from BriefLabelLayer in standard.svg
# These are optimized visual positions for province labels
LABEL_COORDS = {
    "ADR": (849.4, 1100.6),
    "AEG": (1085.7, 1296.4),
    "ALB": (932.4, 1164.4),
    "ANK": (1368.8, 1115.5),
    "APU": (832.3, 1151.7),
    "ARM": (1603, 1145.3),
    "BAL": (827.2, 660.2),
    "BAR": (1077.8, 59.3),
    "BEL": (591.8, 796.2),
    "BER": (760, 728),
    "BLA": (1319.8, 1034.6),
    "BOH": (800.4, 855.8),
    "BOT": (929.6, 538.9),
    "BRE": (430, 866.4),
    "BUD": (953.7, 964.3),
    "BUL": (1021.8, 1109.1),
    "BUR": (530.1, 915.4),
    "CLY": (457.7, 479),
    "CON": (1202.8, 1177.2),
    "DEN": (718.2, 584.3),
    "EAS": (1164.4, 1343.3),
    "EDI": (496, 500.3),
    "ENG": (466.2, 762.6),
    "FIN": (1030.3, 347),
    "GAL": (940.9, 840.9),
    "GAS": (447, 972.8),
    "GRE": (981.4, 1160.2),
    "HEL": (654.2, 665.4),
    "HOL": (608.8, 749.3),
    "ION": (894.1, 1251.7),
    "IRI": (318, 711.4),
    "KIE": (691.9, 753.6),
    "LON": (500.3, 734.4),
    "LVN": (1030.3, 640.8),
    "LVP": (449.2, 564.1),
    "LYO": (525.8, 1107),
    "MAO": (153.3, 815.3),
    "MAR": (572.6, 962.2),
    "MOS": (1568.9, 572.6),
    "MUN": (681.2, 872.8),
    "NAF": (240.6, 1317.7),
    "NAO": (191.6, 268.2),
    "NAP": (832.3, 1211.3),
    "NTH": (595.5, 512),
    "NWG": (664.2, 161.8),
    "NWY": (760, 391.7),
    "PAR": (481.1, 892),
    "PIC": (489.6, 802.5),
    "PIE": (628, 1004.8),
    "POR": (175.1, 1102.1),  # Original has rotate(-60)
    "PRU": (938.8, 687.6),
    "ROM": (729, 1092),  # Original has rotate(45)
    "RUH": (632.2, 826),
    "RUM": (1036.7, 1015.4),
    "SER": (964.3, 1100.6),
    "SEV": (1479.3, 937.7),
    "SIL": (791.9, 779.1),
    "SKA": (754.5, 571.2),  # Original has rotate(75)
    "SMY": (1194.2, 1247.5),
    "SPA": (302.3, 1119.7),
    "STP": (1473.1, 217.1),
    "SWE": (845.1, 391.7),
    "SWI": (649.4, 924.2),
    "SYR": (1639.2, 1292.2),
    "TRI": (826, 966.5),
    "TUN": (634.4, 1351.8),
    "TUS": (706.5, 1069),
    "TYR": (717.4, 936.7),
    "TYS": (710, 1209.1),
    "UKR": (1119.7, 860),
    "VEN": (749.3, 962.2),
    "VIE": (834.5, 919.6),
    "WAL": (436.4, 719.5),
    "WAR": (938.8, 783.4),
    "WES": (400.2, 1209.1),
    "YOR": (496, 653.5),
}

# Province coordinates (x, y) for unit placement
# Extracted from welfare-diplomacy/diplomacy/maps/svg/standard.svg
# Keys are uppercase to match C location names
PROVINCE_COORDS = {
    # Sea zones
    "ADR": (793.5, 1048.0),   # Adriatic Sea
    "AEG": (1043.5, 1230.0),  # Aegean Sea
    "BAL": (878.5, 610.0),    # Baltic Sea
    "BAR": (1162.5, 73.0),    # Barents Sea
    "BLA": (1233.5, 1000.0),  # Black Sea
    "BOT": (941.5, 485.0),    # Gulf of Bothnia
    "EAS": (1218.5, 1311.0),  # Eastern Mediterranean
    "ENG": (394.5, 751.0),    # English Channel
    "HEL": (651.5, 631.0),    # Helgoland Bight
    "ION": (846.5, 1286.0),   # Ionian Sea
    "IRI": (335.5, 661.0),    # Irish Sea
    "LYO": (514.3, 1055.0),   # Gulf of Lyon
    "MAO": (141.8, 835.3),    # Mid-Atlantic Ocean
    "NAO": (180.1, 288.2),    # North Atlantic Ocean
    "NTH": (553.5, 560.0),    # North Sea
    "NWG": (652.7, 181.8),    # Norwegian Sea
    "SKA": (735.5, 518.0),    # Skagerrak
    "TYS": (698.5, 1149.1),   # Tyrrhenian Sea
    "WES": (462.5, 1163.0),   # Western Mediterranean

    # Land/coastal provinces - alphabetical
    "ALB": (906.5, 1113.0),   # Albania
    "ANK": (1301.5, 1110.0),  # Ankara
    "APU": (791.5, 1106.0),   # Apulia
    "ARM": (1484.5, 1090.0),  # Armenia
    "BEL": (561.5, 753.0),    # Belgium
    "BER": (771.5, 690.0),    # Berlin
    "BOH": (806.5, 814.0),    # Bohemia
    "BRE": (404.5, 819.0),    # Brest
    "BUD": (950.5, 904.0),    # Budapest
    "BUL": (1048.5, 1068.0),  # Bulgaria
    "BUR": (559.5, 871.0),    # Burgundy
    "CLY": (436.5, 492.0),    # Clyde
    "CON": (1145.5, 1137.0),  # Constantinople
    "DEN": (703.5, 587.0),    # Denmark
    "EDI": (473.5, 514.0),    # Edinburgh
    "FIN": (988.5, 380.0),    # Finland
    "GAL": (999.5, 831.0),    # Galicia
    "GAS": (422.5, 912.0),    # Gascony
    "GRE": (966.5, 1190.0),   # Greece
    "HOL": (596.5, 711.0),    # Holland
    "KIE": (683.5, 701.0),    # Kiel
    "LON": (488.5, 675.0),    # London
    "LVN": (1025.5, 567.0),   # Livonia
    "LVP": (450.5, 576.0),    # Liverpool
    "MAR": (524.5, 975.0),    # Marseilles
    "MOS": (1200.5, 590.0),   # Moscow
    "MUN": (693.5, 828.0),    # Munich
    "NAF": (325.5, 1281.0),   # North Africa
    "NAP": (806.5, 1170.0),   # Naples
    "NWY": (703.5, 410.0),    # Norway
    "PAR": (488.5, 845.0),    # Paris
    "PIC": (523.5, 781.0),    # Picardy
    "PIE": (630.5, 968.0),    # Piedmont
    "POR": (181.5, 1013.0),   # Portugal
    "PRU": (865.5, 690.0),    # Prussia
    "ROM": (731.5, 1102.0),   # Rome
    "RUH": (636.5, 779.0),    # Ruhr
    "RUM": (1096.5, 967.0),   # Rumania
    "SER": (933.5, 1050.0),   # Serbia
    "SEV": (1284.5, 845.0),   # Sevastopol
    "SIL": (832.5, 769.0),    # Silesia
    "SMY": (1253.5, 1210.0),  # Smyrna
    "SPA": (335.5, 1039.0),   # Spain
    "STP": (1166.5, 405.0),   # St. Petersburg
    "SWE": (829.5, 459.0),    # Sweden
    "SWI": (642.0, 928.0),    # Switzerland
    "SYR": (1452.5, 1206.0),  # Syria
    "TRI": (825.5, 996.0),    # Trieste
    "TUN": (622.5, 1300.0),   # Tunis
    "TUS": (686.5, 1034.0),   # Tuscany
    "TYR": (742.5, 904.0),    # Tyrolia
    "UKR": (1124.5, 800.0),   # Ukraine
    "VEN": (707.5, 994.0),    # Venice
    "VIE": (855.5, 864.0),    # Vienna
    "WAL": (428.5, 658.0),    # Wales
    "WAR": (983.5, 740.0),    # Warsaw
    "YOR": (492.5, 616.0),    # Yorkshire

    # Split coast variants
    "BUL/EC": (1127.0, 1067.0),  # Bulgaria East Coast
    "BUL/SC": (1070.0, 1140.0),  # Bulgaria South Coast
    "SPA/NC": (289.0, 965.0),    # Spain North Coast
    "SPA/SC": (291.0, 1166.0),   # Spain South Coast
    "STP/NC": (1218.0, 222.0),   # St. Petersburg North Coast
    "STP/SC": (1066.0, 487.0),   # St. Petersburg South Coast
}

# Dislodged unit coordinates (slightly offset from normal)
DISLODGED_COORDS = {
    "ADR": (782.0, 1038.0),
    "AEG": (1032.0, 1220.0),
    "ALB": (895.0, 1103.0),
    "ANK": (1290.0, 1100.0),
    "APU": (780.0, 1096.0),
    "ARM": (1473.0, 1080.0),
    "BAL": (867.0, 600.0),
    "BAR": (1151.0, 63.0),
    "BEL": (550.0, 743.0),
    "BER": (760.0, 680.0),
    "BLA": (1222.0, 990.0),
    "BOH": (795.0, 804.0),
    "BOT": (930.0, 475.0),
    "BRE": (393.0, 809.0),
    "BUD": (939.0, 894.0),
    "BUL": (1037.0, 1058.0),
    "BUR": (548.0, 861.0),
    "CLY": (425.0, 482.0),
    "CON": (1134.0, 1127.0),
    "DEN": (692.0, 577.0),
    "EAS": (1207.0, 1301.0),
    "EDI": (462.0, 504.0),
    "ENG": (383.0, 741.0),
    "FIN": (977.0, 370.0),
    "GAL": (988.0, 821.0),
    "GAS": (411.0, 902.0),
    "GRE": (955.0, 1180.0),
    "HEL": (640.0, 621.0),
    "HOL": (585.0, 701.0),
    "ION": (835.0, 1276.0),
    "IRI": (324.0, 651.0),
    "KIE": (672.0, 691.0),
    "LON": (477.0, 665.0),
    "LVN": (1014.0, 557.0),
    "LVP": (439.0, 566.0),
    "LYO": (502.8, 1045.0),
    "MAO": (130.3, 825.3),
    "MAR": (513.0, 965.0),
    "MOS": (1189.0, 580.0),
    "MUN": (682.0, 818.0),
    "NAF": (314.0, 1271.0),
    "NAO": (168.6, 278.2),
    "NAP": (795.0, 1160.0),
    "NTH": (542.0, 550.0),
    "NWG": (641.2, 171.8),
    "NWY": (692.0, 400.0),
    "PAR": (477.0, 835.0),
    "PIC": (512.0, 771.0),
    "PIE": (619.0, 958.0),
    "POR": (170.0, 1003.0),
    "PRU": (854.0, 680.0),
    "ROM": (720.0, 1092.0),
    "RUH": (625.0, 769.0),
    "RUM": (1085.0, 957.0),
    "SER": (922.0, 1040.0),
    "SEV": (1273.0, 835.0),
    "SIL": (821.0, 759.0),
    "SKA": (724.0, 508.0),
    "SMY": (1242.0, 1200.0),
    "SPA": (324.0, 1029.0),
    "STP": (1155.0, 395.0),
    "SWE": (818.0, 449.0),
    "SWI": (630.5, 918.0),
    "SYR": (1441.0, 1196.0),
    "TRI": (814.0, 986.0),
    "TUN": (611.0, 1290.0),
    "TUS": (675.0, 1024.0),
    "TYR": (731.0, 894.0),
    "TYS": (687.0, 1139.1),
    "UKR": (1113.0, 790.0),
    "VEN": (696.0, 984.0),
    "VIE": (844.0, 854.0),
    "WAL": (417.0, 648.0),
    "WAR": (972.0, 730.0),
    "WES": (451.0, 1153.0),
    "YOR": (481.0, 606.0),
    "BUL/EC": (1115.5, 1057.0),
    "BUL/SC": (1058.5, 1130.0),
    "SPA/NC": (277.5, 955.0),
    "SPA/SC": (279.5, 1156.0),
    "STP/NC": (1206.5, 212.0),
    "STP/SC": (1054.5, 477.0),
}

# Power colors (RGB tuples for Raylib)
# From SVG jdipNS:POWERCOLORS and typical Diplomacy conventions
POWER_COLORS = {
    0: (196, 143, 133),   # Austria - pinkish brown
    1: (148, 0, 211),     # England - dark violet
    2: (65, 105, 225),    # France - royal blue
    3: (139, 119, 101),   # Germany - grayish brown
    4: (34, 139, 34),     # Italy - forest green
    5: (80, 80, 90),      # Russia - dark grey
    6: (255, 215, 0),     # Turkey - gold
    -1: (128, 128, 128),  # Neutral - gray
}

# Power names (indexed by power ID)
POWER_NAMES = [
    "AUSTRIA",
    "ENGLAND",
    "FRANCE",
    "GERMANY",
    "ITALY",
    "RUSSIA",
    "TURKEY",
]

# Power abbreviations
POWER_ABBREV = ["A", "E", "F", "G", "I", "R", "T"]

# Location index to name mapping (from diplomacy_map.c)
# C uses indices 0-81
LOC_INDEX_TO_NAME = {
    0: "ADR", 1: "AEG", 2: "ALB", 3: "ANK", 4: "APU", 5: "ARM",
    6: "BAL", 7: "BAR", 8: "BEL", 9: "BER", 10: "BLA", 11: "BOH",
    12: "BOT", 13: "BRE", 14: "BUD", 15: "BUL", 16: "BUR", 17: "CLY",
    18: "CON", 19: "DEN", 20: "EAS", 21: "EDI", 22: "ENG", 23: "FIN",
    24: "GAL", 25: "GAS", 26: "GRE", 27: "HEL", 28: "HOL", 29: "ION",
    30: "IRI", 31: "KIE", 32: "LON", 33: "LVN", 34: "LVP", 35: "LYO",
    36: "MAO", 37: "MAR", 38: "MOS", 39: "MUN", 40: "NAF", 41: "NAO",
    42: "NAP", 43: "NTH", 44: "NWG", 45: "NWY", 46: "PAR", 47: "PIC",
    48: "PIE", 49: "POR", 50: "PRU", 51: "ROM", 52: "RUH", 53: "RUM",
    54: "SER", 55: "SEV", 56: "SIL", 57: "SKA", 58: "SMY", 59: "SPA",
    60: "STP", 61: "SWE", 62: "SWI", 63: "SYR", 64: "TRI", 65: "TUN",
    66: "TUS", 67: "TYR", 68: "TYS", 69: "UKR", 70: "VEN", 71: "VIE",
    72: "WAL", 73: "WAR", 74: "WES", 75: "YOR",
    76: "BUL/EC", 77: "BUL/SC", 78: "SPA/NC", 79: "SPA/SC",
    80: "STP/NC", 81: "STP/SC",
}

# Reverse mapping
LOC_NAME_TO_INDEX = {v: k for k, v in LOC_INDEX_TO_NAME.items()}

# Phase names
PHASE_NAMES = {
    0: "SPRING MOVEMENT",
    1: "SPRING RETREAT",
    2: "FALL MOVEMENT",
    3: "FALL RETREAT",
    4: "WINTER ADJUSTMENT",
    5: "COMPLETED",
}

# Supply center locations (for drawing ownership markers)
SUPPLY_CENTERS = [
    "ANK", "BEL", "BER", "BRE", "BUD", "BUL", "CON", "DEN", "EDI",
    "GRE", "HOL", "KIE", "LON", "LVP", "MAR", "MOS", "MUN", "NAP",
    "NWY", "PAR", "POR", "ROM", "RUM", "SER", "SEV", "SMY", "SPA",
    "STP", "SWE", "TRI", "TUN", "VEN", "VIE", "WAR",
]

# Home centers per power
HOME_CENTERS = {
    0: ["BUD", "TRI", "VIE"],           # Austria
    1: ["EDI", "LON", "LVP"],           # England
    2: ["BRE", "MAR", "PAR"],           # France
    3: ["BER", "KIE", "MUN"],           # Germany
    4: ["NAP", "ROM", "VEN"],           # Italy
    5: ["MOS", "SEV", "STP", "WAR"],    # Russia (4 home centers)
    6: ["ANK", "CON", "SMY"],           # Turkey
}


def get_coords(loc_name, is_dislodged=False):
    """Get coordinates for a location name.

    Args:
        loc_name: Location name (e.g., "PAR", "STP/NC")
        is_dislodged: If True, return dislodged unit position

    Returns:
        Tuple (x, y) or (0, 0) if not found
    """
    name = loc_name.upper()
    if is_dislodged:
        return DISLODGED_COORDS.get(name, PROVINCE_COORDS.get(name, (0, 0)))
    return PROVINCE_COORDS.get(name, (0, 0))


def get_coords_by_index(loc_index, is_dislodged=False):
    """Get coordinates for a location index.

    Args:
        loc_index: Location index (0-81)
        is_dislodged: If True, return dislodged unit position

    Returns:
        Tuple (x, y) or (0, 0) if not found
    """
    name = LOC_INDEX_TO_NAME.get(loc_index, "")
    return get_coords(name, is_dislodged)


def get_power_color(power_id):
    """Get RGB color tuple for a power.

    Args:
        power_id: Power index (0-6) or -1 for neutral

    Returns:
        Tuple (r, g, b)
    """
    return POWER_COLORS.get(power_id, POWER_COLORS[-1])


def get_location_name(loc_index):
    """Get location name from index.

    Args:
        loc_index: Location index (0-81)

    Returns:
        Location name string or empty string if not found
    """
    return LOC_INDEX_TO_NAME.get(loc_index, "")
