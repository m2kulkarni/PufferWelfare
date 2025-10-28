#include "diplomacy.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>

// ============================================================================
// Utility Functions
// ============================================================================

const char* phase_to_string(PhaseType phase) {
    switch (phase) {
        case PHASE_SPRING_MOVEMENT: return "SPRING 1901 MOVEMENT";
        case PHASE_SPRING_RETREAT: return "SPRING 1901 RETREAT";
        case PHASE_FALL_MOVEMENT: return "FALL 1901 MOVEMENT";
        case PHASE_FALL_RETREAT: return "FALL 1901 RETREAT";
        case PHASE_WINTER_ADJUSTMENT: return "WINTER 1901 ADJUSTMENT";
        case PHASE_COMPLETED: return "COMPLETED";
        default: return "UNKNOWN";
    }
}

int find_location_by_name(Map* map, const char* name) {
    for (int i = 0; i < map->num_locations; i++) {
        if (strcmp(map->locations[i].name, name) == 0) {
            return i;
        }
    }
    return -1;  // Not found
}

// Get the parent location (base location without coast specification)
int get_parent_location(Map* map, int loc_idx) {
    if (loc_idx < 0 || loc_idx >= map->num_locations) {
        return -1;
    }
    int parent = map->locations[loc_idx].parent_location;
    if (parent == -1) {
        return loc_idx;  // This is already a parent location
    }
    return parent;
}

// Get all coast variants of a location (including the location itself)
void find_coasts(Map* map, int loc_idx, int* coasts, int* num_coasts) {
    *num_coasts = 0;
    if (loc_idx < 0 || loc_idx >= map->num_locations) {
        return;
    }

    // Get the parent location
    int parent = get_parent_location(map, loc_idx);

    // Find all locations that have this parent (or are the parent)
    for (int i = 0; i < map->num_locations; i++) {
        if (i == parent || map->locations[i].parent_location == parent) {
            coasts[*num_coasts] = i;
            (*num_coasts)++;
        }
    }
}

// Check if a location requires coast specification for fleets
// Auto-determine coast for fleet move if only one reachable coast
// Returns the specific coast index, or -1 if ambiguous or not found
int default_coast(Map* map, int from_loc, const char* dest_name) {
    if (from_loc < 0 || from_loc >= map->num_locations) {
        return -1;
    }

    // First, try to find exact match
    int exact = find_location_by_name(map, dest_name);
    if (exact != -1) {
        return exact;
    }

    // If not found, check if it's a split coast location without coast specified
    // Try to find all variants of this location
    char base_name[4];
    if (strlen(dest_name) == 3) {
        strncpy(base_name, dest_name, 3);
        base_name[3] = '\0';

        // Find all locations that match this base name
        int matching_coasts[10];
        int num_matching = 0;

        for (int i = 0; i < map->num_locations; i++) {
            if (strncmp(map->locations[i].name, base_name, 3) == 0) {
                // Check if this location is reachable from from_loc
                for (int j = 0; j < map->locations[from_loc].num_adjacent; j++) {
                    if (map->locations[from_loc].adjacencies[j] == i) {
                        matching_coasts[num_matching++] = i;
                        break;
                    }
                }
            }
        }

        // If exactly one reachable coast, return it
        if (num_matching == 1) {
            return matching_coasts[0];
        }

        // If multiple or none, it's ambiguous
        return -1;
    }

    return -1;
}

int get_unit_at_location(GameState* game, int location) {
    for (int p = 0; p < MAX_POWERS; p++) {
        for (int u = 0; u < game->powers[p].num_units; u++) {
            if (game->powers[p].units[u].location == location) {
                return p;  // Return power ID
            }
        }
    }
    return -1;  // No unit at location
}

int can_move(Map* map, UnitType unit_type, int from_loc, int to_loc) {
    if (from_loc < 0 || from_loc >= map->num_locations ||
        to_loc < 0 || to_loc >= map->num_locations) {
        return 0;
    }
    return map->adjacency_cache[unit_type][from_loc][to_loc];
}

// ============================================================================
// Game State Query Functions (for testing)
// ============================================================================

int get_current_year(GameState* game) {
    return game->year;
}

PhaseType get_current_phase(GameState* game) {
    return game->phase;
}

int get_num_units(GameState* game, int power_id) {
    if (power_id < 0 || power_id >= MAX_POWERS) {
        return 0;
    }
    return game->powers[power_id].num_units;
}

void get_unit_info(GameState* game, int power_id, int unit_idx, UnitType* type, int* location) {
    if (power_id < 0 || power_id >= MAX_POWERS) {
        return;
    }
    if (unit_idx < 0 || unit_idx >= game->powers[power_id].num_units) {
        return;
    }
    *type = game->powers[power_id].units[unit_idx].type;
    *location = game->powers[power_id].units[unit_idx].location;
}

int get_num_centers(GameState* game, int power_id) {
    if (power_id < 0 || power_id >= MAX_POWERS) {
        return 0;
    }
    return game->powers[power_id].num_centers;
}

void get_center_locations(GameState* game, int power_id, int* centers, int* num_centers) {
    if (power_id < 0 || power_id >= MAX_POWERS) {
        *num_centers = 0;
        return;
    }
    *num_centers = game->powers[power_id].num_centers;
    for (int i = 0; i < *num_centers; i++) {
        centers[i] = game->powers[power_id].centers[i];
    }
}

int get_welfare_points(GameState* game, int power_id) {
    if (power_id < 0 || power_id >= MAX_POWERS) {
        return 0;
    }
    return game->powers[power_id].welfare_points;
}

const char* get_location_name(Map* map, int location_idx) {
    if (location_idx < 0 || location_idx >= map->num_locations) {
        return "";
    }
    return map->locations[location_idx].name;
}

int get_num_locations(Map* map) {
    return map->num_locations;
}

LocationType get_location_type(Map* map, int location_idx) {
    if (location_idx < 0 || location_idx >= map->num_locations) {
        return LOC_LAND;  // Default
    }
    return map->locations[location_idx].type;
}

int is_supply_center(Map* map, int location_idx) {
    if (location_idx < 0 || location_idx >= map->num_locations) {
        return 0;
    }
    return map->locations[location_idx].has_supply_center;
}

int get_num_orders(GameState* game, int power_id) {
    if (power_id < 0 || power_id >= MAX_POWERS) {
        return 0;
    }
    // Return from persistent storage (survives phase transitions)
    return game->last_num_orders[power_id];
}

OrderResult get_order_result(GameState* game, int power_id, int order_idx) {
    if (power_id < 0 || power_id >= MAX_POWERS) {
        return RESULT_NONE;
    }
    if (order_idx < 0 || order_idx >= game->last_num_orders[power_id]) {
        return RESULT_NONE;
    }
    // Return from persistent storage (survives phase transitions)
    return game->last_results[power_id][order_idx];
}

// ============================================================================
// Map Initialization - Generated from standard.map
// ============================================================================


// ============================================================================
// Game Initialization and Lifecycle
// ============================================================================

void init_game(GameState* game, Map* map, int welfare_mode, int max_years) {
    memset(game, 0, sizeof(GameState));

    game->map = map;
    game->year = 1901;
    game->phase = PHASE_SPRING_MOVEMENT;
    game->welfare_mode = welfare_mode;
    game->max_years = max_years;
    game->is_game_over = 0;

    // Initialize powers
    for (int p = 0; p < MAX_POWERS; p++) {
        game->powers[p].power_id = p;
        strcpy(game->powers[p].name, map->power_names[p]);
        strcpy(game->powers[p].abbrev, map->power_abbrev[p]);
        game->powers[p].welfare_points = 0;
        game->powers[p].num_centers = 0;
        game->powers[p].num_units = 0;
        game->powers[p].num_orders = 0;
        game->powers[p].num_retreats = 0;
        game->powers[p].adjustment = 0;
    }

    // Set up initial unit positions and supply centers
    // Power IDs: 0=AUSTRIA, 1=ENGLAND, 2=FRANCE, 3=GERMANY, 4=ITALY, 5=RUSSIA, 6=TURKEY

    // AUSTRIA (0): F TRI, A VIE, A BUD
    game->powers[0].units[0] = (Unit){UNIT_FLEET, 64, 0, 0}; // F TRI
    game->powers[0].units[1] = (Unit){UNIT_ARMY, 71, 0, 0};  // A VIE
    game->powers[0].units[2] = (Unit){UNIT_ARMY, 14, 0, 0};  // A BUD
    game->powers[0].num_units = 3;
    game->powers[0].centers[0] = 64; // TRI
    game->powers[0].centers[1] = 71; // VIE
    game->powers[0].centers[2] = 14; // BUD
    game->powers[0].num_centers = 3;

    // ENGLAND (1): F LON, F EDI, A LVP
    game->powers[1].units[0] = (Unit){UNIT_FLEET, 32, 1, 0}; // F LON
    game->powers[1].units[1] = (Unit){UNIT_FLEET, 21, 1, 0}; // F EDI
    game->powers[1].units[2] = (Unit){UNIT_ARMY, 34, 1, 0};  // A LVP
    game->powers[1].num_units = 3;
    game->powers[1].centers[0] = 32; // LON
    game->powers[1].centers[1] = 21; // EDI
    game->powers[1].centers[2] = 34; // LVP
    game->powers[1].num_centers = 3;

    // FRANCE (2): F BRE, A PAR, A MAR
    game->powers[2].units[0] = (Unit){UNIT_FLEET, 13, 2, 0}; // F BRE
    game->powers[2].units[1] = (Unit){UNIT_ARMY, 46, 2, 0};  // A PAR
    game->powers[2].units[2] = (Unit){UNIT_ARMY, 37, 2, 0};  // A MAR
    game->powers[2].num_units = 3;
    game->powers[2].centers[0] = 13; // BRE
    game->powers[2].centers[1] = 46; // PAR
    game->powers[2].centers[2] = 37; // MAR
    game->powers[2].num_centers = 3;

    // GERMANY (3): F KIE, A BER, A MUN
    game->powers[3].units[0] = (Unit){UNIT_FLEET, 31, 3, 0}; // F KIE
    game->powers[3].units[1] = (Unit){UNIT_ARMY, 9, 3, 0};   // A BER
    game->powers[3].units[2] = (Unit){UNIT_ARMY, 39, 3, 0};  // A MUN
    game->powers[3].num_units = 3;
    game->powers[3].centers[0] = 31; // KIE
    game->powers[3].centers[1] = 9;  // BER
    game->powers[3].centers[2] = 39; // MUN
    game->powers[3].num_centers = 3;

    // ITALY (4): F NAP, A ROM, A VEN
    game->powers[4].units[0] = (Unit){UNIT_FLEET, 42, 4, 0}; // F NAP
    game->powers[4].units[1] = (Unit){UNIT_ARMY, 51, 4, 0};  // A ROM
    game->powers[4].units[2] = (Unit){UNIT_ARMY, 70, 4, 0};  // A VEN
    game->powers[4].num_units = 3;
    game->powers[4].centers[0] = 42; // NAP
    game->powers[4].centers[1] = 51; // ROM
    game->powers[4].centers[2] = 70; // VEN
    game->powers[4].num_centers = 3;

    // RUSSIA (5): F SEV, F STP(sc), A MOS, A WAR
    game->powers[5].units[0] = (Unit){UNIT_FLEET, 55, 5, 0}; // F SEV
    game->powers[5].units[1] = (Unit){UNIT_FLEET, 60, 5, 0}; // F STP
    game->powers[5].units[2] = (Unit){UNIT_ARMY, 38, 5, 0};  // A MOS
    game->powers[5].units[3] = (Unit){UNIT_ARMY, 73, 5, 0};  // A WAR
    game->powers[5].num_units = 4;
    game->powers[5].centers[0] = 55; // SEV
    game->powers[5].centers[1] = 60; // STP
    game->powers[5].centers[2] = 38; // MOS
    game->powers[5].centers[3] = 73; // WAR
    game->powers[5].num_centers = 4;

    // TURKEY (6): F ANK, A CON, A SMY
    game->powers[6].units[0] = (Unit){UNIT_FLEET, 3, 6, 0};  // F ANK
    game->powers[6].units[1] = (Unit){UNIT_ARMY, 18, 6, 0};  // A CON
    game->powers[6].units[2] = (Unit){UNIT_ARMY, 58, 6, 0};  // A SMY
    game->powers[6].num_units = 3;
    game->powers[6].centers[0] = 3;  // ANK
    game->powers[6].centers[1] = 18; // CON
    game->powers[6].centers[2] = 58; // SMY
    game->powers[6].num_centers = 3;
}

void reset_game(GameState* game) {
    int welfare_mode = game->welfare_mode;
    int max_years = game->max_years;
    Map* map = game->map;

    init_game(game, map, welfare_mode, max_years);
}

void free_game(GameState* game) {
    // No dynamic allocation in GameState currently, but keep for future
    (void)game;
}

// ============================================================================
// Order Handling
// ============================================================================

int parse_order(const char* order_str, Order* order, GameState* game) {
    // Parse standard Diplomacy order notation
    // Format examples:
    // "A PAR - MAR" (move)
    // "A PAR H" or "A PAR HOLDS" (hold)
    // "A PAR S A MAR - BUR" (support move)
    // "A PAR S A MAR" (support hold)
    // "F ENG C A WAL - BRE" (convoy)

    if (!order_str || !order || !game) {
        return -1;
    }

    memset(order, 0, sizeof(Order));

    char buffer[256];
    strncpy(buffer, order_str, sizeof(buffer) - 1);
    buffer[sizeof(buffer) - 1] = '\0';

    // Convert to uppercase for easier parsing
    for (char* p = buffer; *p; p++) {
        *p = toupper(*p);
    }

    // Parse unit type (A or F)
    char* token = strtok(buffer, " ");
    if (!token) return -1;

    if (token[0] == 'A') {
        order->unit_type = UNIT_ARMY;
    } else if (token[0] == 'F') {
        order->unit_type = UNIT_FLEET;
    } else {
        return -1;  // Invalid unit type
    }

    // Parse unit location
    token = strtok(NULL, " ");
    if (!token) return -1;

    int unit_loc = find_location_by_name(game->map, token);

    // If not found and this is an army on a split coast territory,
    // try the generic land version (lowercase name)
    if (unit_loc < 0 && order->unit_type == UNIT_ARMY) {
        // Try lowercase version for generic land connections (stp, bul, spa)
        char lower_name[8];
        strncpy(lower_name, token, sizeof(lower_name) - 1);
        lower_name[sizeof(lower_name) - 1] = '\0';
        for (char* p = lower_name; *p; p++) {
            *p = tolower(*p);
        }
        unit_loc = find_location_by_name(game->map, lower_name);
    }

    if (unit_loc < 0) return -1;
    order->unit_location = unit_loc;

    // Parse order type
    token = strtok(NULL, " ");
    if (!token) return -1;

    if (strcmp(token, "H") == 0 || strcmp(token, "HOLDS") == 0) {
        // HOLD order
        order->type = ORDER_HOLD;
        return 0;

    } else if (strcmp(token, "-") == 0 || strcmp(token, "->") == 0) {
        // MOVE order
        order->type = ORDER_MOVE;

        // Parse destination
        token = strtok(NULL, " ");
        if (!token) return -1;

        int dest_loc = find_location_by_name(game->map, token);

        // If not found and this is a fleet, try default_coast
        if (dest_loc < 0 && order->unit_type == UNIT_FLEET) {
            dest_loc = default_coast(game->map, order->unit_location, token);
        }

        if (dest_loc < 0) return -1;  // Invalid or ambiguous
        order->target_location = dest_loc;
        return 0;

    } else if (strcmp(token, "S") == 0 || strcmp(token, "SUPPORT") == 0 ||
               strcmp(token, "SUPPORTS") == 0) {
        // SUPPORT order

        // Parse supported unit type
        token = strtok(NULL, " ");
        if (!token) return -1;

        // Skip unit type (already know from next token)
        // Parse supported unit location
        token = strtok(NULL, " ");
        if (!token) return -1;

        int supported_loc = find_location_by_name(game->map, token);
        if (supported_loc < 0) return -1;
        order->target_unit_location = supported_loc;

        // Check if it's support to hold or support to move
        token = strtok(NULL, " ");
        if (token && (strcmp(token, "-") == 0 || strcmp(token, "->") == 0)) {
            // Support to move
            order->type = ORDER_SUPPORT_MOVE;
            token = strtok(NULL, " ");
            if (!token) return -1;

            int dest_loc = find_location_by_name(game->map, token);
            if (dest_loc < 0) return -1;
            order->dest_location = dest_loc;
        } else {
            // Support to hold (no destination)
            order->type = ORDER_SUPPORT_HOLD;
            order->dest_location = supported_loc;
        }
        return 0;

    } else if (strcmp(token, "C") == 0 || strcmp(token, "CONVOY") == 0 ||
               strcmp(token, "CONVOYS") == 0) {
        // CONVOY order
        order->type = ORDER_CONVOY;

        // Parse convoyed unit type (should be A)
        token = strtok(NULL, " ");
        if (!token) return -1;

        // Parse convoyed unit location
        token = strtok(NULL, " ");
        if (!token) return -1;

        int convoyed_loc = find_location_by_name(game->map, token);
        if (convoyed_loc < 0) return -1;
        order->target_unit_location = convoyed_loc;

        // Parse "-"
        token = strtok(NULL, " ");
        if (!token || (strcmp(token, "-") != 0 && strcmp(token, "->") != 0)) {
            return -1;
        }

        // Parse destination
        token = strtok(NULL, " ");
        if (!token) return -1;

        int dest_loc = find_location_by_name(game->map, token);
        if (dest_loc < 0) return -1;
        order->dest_location = dest_loc;
        return 0;

    } else if (strcmp(token, "B") == 0 || strcmp(token, "BUILD") == 0 ||
               strcmp(token, "BUILDS") == 0) {
        // BUILD order
        order->type = ORDER_BUILD;
        return 0;

    } else if (strcmp(token, "D") == 0 || strcmp(token, "DISBAND") == 0 ||
               strcmp(token, "DISBANDS") == 0 || strcmp(token, "REMOVE") == 0) {
        // DISBAND order
        order->type = ORDER_DISBAND;
        return 0;

    } else if (strcmp(token, "R") == 0 || strcmp(token, "RETREAT") == 0 ||
               strcmp(token, "RETREATS") == 0) {
        // RETREAT order: "F TRI R ALB"
        order->type = ORDER_RETREAT;

        // Parse retreat destination
        token = strtok(NULL, " ");
        if (!token) return -1;

        int dest_loc = find_location_by_name(game->map, token);
        if (dest_loc < 0) return -1;
        order->target_location = dest_loc;
        return 0;

    } else {
        return -1;  // Unknown order type
    }
}

int validate_order(GameState* game, int power_id, const Order* order) {
    // Validate that an order is legal for the current game state
    // Returns 0 if valid, -1 if invalid

    if (!game || !order || power_id < 0 || power_id >= MAX_POWERS) {
        return -1;
    }

    Power* power = &game->powers[power_id];
    Map* map = game->map;
    PhaseType phase = game->phase;

    // Phase-specific order validation
    // During retreat phase, only RETREAT and DISBAND orders are allowed
    if (phase == PHASE_SPRING_RETREAT || phase == PHASE_FALL_RETREAT) {
        if (order->type != ORDER_RETREAT && order->type != ORDER_DISBAND) {
            return -1;  // Invalid order type for retreat phase
        }
        // For retreat orders, validate against dislodged units
        // Check if this unit is in the retreat list (must be dislodged)
        int found = 0;
        for (int r = 0; r < power->num_retreats; r++) {
            if (power->retreats[r].from_location == order->unit_location &&
                power->retreats[r].type == order->unit_type) {
                found = 1;
                break;
            }
        }
        if (!found) {
            return -1;  // Unit not dislodged, cannot retreat
        }
        return 0;  // Valid retreat/disband order
    }

    // During adjustment phase, only BUILD and DISBAND orders are allowed
    if (phase == PHASE_WINTER_ADJUSTMENT) {
        if (order->type != ORDER_BUILD && order->type != ORDER_DISBAND) {
            return -1;  // Invalid order type for adjustment phase
        }
        // Additional validation for builds/disbands would go here
        return 0;
    }

    // Movement phase - normal validation
    // Find unit at the specified location
    int unit_idx = -1;
    for (int i = 0; i < power->num_units; i++) {
        if (power->units[i].location == order->unit_location) {
            unit_idx = i;
            break;
        }
    }

    // Check unit exists and belongs to this power
    if (unit_idx < 0) {
        return -1;  // No unit at this location
    }

    Unit* unit = &power->units[unit_idx];

    // Check unit type matches
    if (unit->type != order->unit_type) {
        return -1;  // Unit type mismatch
    }

    // Validate based on order type
    switch (order->type) {
        case ORDER_HOLD:
            // HOLD is always valid
            return 0;

        case ORDER_MOVE:
            // Check destination is adjacent and reachable by unit type
            if (order->target_location < 0 || order->target_location >= map->num_locations) {
                return -1;  // Invalid destination
            }
            // Check not moving to same location
            if (order->target_location == unit->location) {
                return -1;  // Cannot move to own sector
            }
            // Check if move is adjacent (normal move)
            if (can_move(map, unit->type, unit->location, order->target_location)) {
                return 0;  // Valid adjacent move
            }
            // If not adjacent and unit is army, check for convoy path
            if (unit->type == UNIT_ARMY && is_convoyed_move(game, unit->location, order->target_location)) {
                return 0;  // Valid convoy move
            }
            return -1;  // Not adjacent and no convoy path

        case ORDER_SUPPORT_HOLD:
        case ORDER_SUPPORT_MOVE:
            // Check target unit location exists
            if (order->target_unit_location < 0 || order->target_unit_location >= map->num_locations) {
                return -1;
            }
            // Check supporting unit can reach the target location or destination
            // (supporting unit must be adjacent to either the supported unit or the destination)
            int can_support = 0;
            if (can_move(map, unit->type, unit->location, order->target_unit_location)) {
                can_support = 1;  // Adjacent to supported unit
            }
            if (order->dest_location >= 0 && order->dest_location < map->num_locations) {
                if (can_move(map, unit->type, unit->location, order->dest_location)) {
                    can_support = 1;  // Adjacent to destination
                }
            }
            if (!can_support) {
                return -1;  // Can't support this move
            }
            return 0;

        case ORDER_CONVOY:
            // Convoy must be given by a fleet
            if (unit->type != UNIT_FLEET) {
                return -1;  // Only fleets can convoy
            }
            // Check fleet is in WATER (not COAST) - can't convoy from coastal areas
            if (!can_fleet_convoy(map, unit->location)) {
                return -1;  // Fleet must be in water to convoy
            }
            // Check convoyed unit location exists
            if (order->target_unit_location < 0 || order->target_unit_location >= map->num_locations) {
                return -1;
            }
            // Check destination exists
            if (order->dest_location < 0 || order->dest_location >= map->num_locations) {
                return -1;
            }
            // Convoy order is valid if fleet can convoy
            return 0;

        case ORDER_BUILD:
            // Can only build in home centers during adjustment phase
            if (game->phase != PHASE_WINTER_ADJUSTMENT) {
                return -1;  // Wrong phase
            }
            // TODO: Check location is home center and unoccupied
            return 0;

        case ORDER_DISBAND:
            // Can always disband own units (in welfare diplomacy)
            // In standard diplomacy, only during adjustment phase if over limit
            if (!game->welfare_mode && game->phase != PHASE_WINTER_ADJUSTMENT) {
                return -1;  // Wrong phase for standard diplomacy
            }
            return 0;

        default:
            return -1;  // Unknown order type
    }
}

void get_possible_orders(GameState* game, int power_id, int location,
                         char orders[][MAX_ORDER_LENGTH], int* num_orders) {
    *num_orders = 0;
    if (!game || power_id < 0 || power_id >= MAX_POWERS) return;
    Power* power = &game->powers[power_id];
    int unit_idx = -1;
    for (int i = 0; i < power->num_units; i++) {
        if (power->units[i].location == location) {
            unit_idx = i;
            break;
        }
    }
    if (unit_idx < 0) return; // no unit
    Unit* unit = &power->units[unit_idx];

    // HOLD always available
    snprintf(orders[*num_orders], MAX_ORDER_LENGTH, "%s %s H",
             unit->type == UNIT_ARMY ? "A" : "F",
             game->map->locations[unit->location].name);
    (*num_orders)++;

    // MOVE to any adjacent and reachable location
    for (int a = 0; a < game->map->locations[location].num_adjacent; a++) {
        int to = game->map->locations[location].adjacencies[a];
        if (can_move(game->map, unit->type, location, to)) {
            snprintf(orders[*num_orders], MAX_ORDER_LENGTH, "%s %s - %s",
                     unit->type == UNIT_ARMY ? "A" : "F",
                     game->map->locations[unit->location].name,
                     game->map->locations[to].name);
            (*num_orders)++;
            if (*num_orders >= MAX_UNITS) break; // cap defensively
        }
    }
}

// ============================================================================
// Convoy Pathfinding
// ============================================================================

// Check if a fleet at given location can convoy (must be in water, not coast)
int can_fleet_convoy(Map* map, int location) {
    if (location < 0 || location >= map->num_locations) {
        return 0;
    }

    Location* loc = &map->locations[location];
    // Fleet can convoy only if in WATER (not COAST, not LAND)
    return (loc->type == LOC_WATER);
}

// BFS to find if there's a convoy path from start to end through convoying fleets
// Returns 1 if path exists, 0 otherwise
// convoying_fleets: array of locations where fleets are offering to convoy
// num_convoying_fleets: number of such fleets
int find_convoy_path(Map* map, int start, int end,
                     int* convoying_fleets, int num_convoying_fleets) {
    if (start == end) return 0;  // Can't convoy to same location

    // Check if start and end are coastal (armies can only convoy from/to coast)
    Location* start_loc = &map->locations[start];
    Location* end_loc = &map->locations[end];

    if (start_loc->type != LOC_COAST && start_loc->type != LOC_PORT) return 0;
    if (end_loc->type != LOC_COAST && end_loc->type != LOC_PORT) return 0;

    // BFS to find path through convoying fleets
    // Queue for BFS
    int queue[MAX_LOCATIONS];
    int visited[MAX_LOCATIONS] = {0};
    int queue_start = 0, queue_end = 0;

    // Start with convoying fleets adjacent to start location
    for (int i = 0; i < num_convoying_fleets; i++) {
        int fleet_loc = convoying_fleets[i];

        // Check if this fleet is adjacent to start
        int adjacent = 0;
        for (int j = 0; j < start_loc->num_adjacent; j++) {
            if (start_loc->adjacencies[j] == fleet_loc) {
                adjacent = 1;
                break;
            }
        }

        if (adjacent && can_fleet_convoy(map, fleet_loc)) {
            queue[queue_end++] = fleet_loc;
            visited[fleet_loc] = 1;
        }
    }

    // BFS through convoying fleets
    while (queue_start < queue_end) {
        int current = queue[queue_start++];
        Location* current_loc = &map->locations[current];

        // Check if this fleet is adjacent to destination
        for (int i = 0; i < current_loc->num_adjacent; i++) {
            if (current_loc->adjacencies[i] == end) {
                return 1;  // Found a path!
            }
        }

        // Expand to adjacent convoying fleets
        for (int i = 0; i < current_loc->num_adjacent; i++) {
            int adj = current_loc->adjacencies[i];

            if (visited[adj]) continue;

            // Check if there's a convoying fleet at this location
            for (int j = 0; j < num_convoying_fleets; j++) {
                if (convoying_fleets[j] == adj && can_fleet_convoy(map, adj)) {
                    queue[queue_end++] = adj;
                    visited[adj] = 1;
                    break;
                }
            }
        }
    }

    return 0;  // No path found
}

// Get list of fleets offering to convoy an army from start to end
// Returns number of convoying fleets found
// Check if an army move should be via convoy (has valid convoy path)
int is_convoyed_move(GameState* game, int from, int to) {
    int convoying_fleets[MAX_LOCATIONS];
    int count = 0;

    // Find all fleets offering to convoy this move
    for (int p = 0; p < MAX_POWERS; p++) {
        Power* power = &game->powers[p];
        for (int o = 0; o < power->num_orders; o++) {
            Order* order = &power->orders[o];
            if (order->type == ORDER_CONVOY &&
                order->target_unit_location == from &&
                order->dest_location == to) {
                int fleet_loc = order->unit_location;
                if (can_fleet_convoy(game->map, fleet_loc)) {
                    convoying_fleets[count++] = fleet_loc;
                    if (count >= MAX_LOCATIONS) break;
                }
            }
        }
    }

    if (count == 0) {
        return 0;
    }

    return find_convoy_path(game->map, from, to, convoying_fleets, count);
}

int detect_paradox(GameState* game, int starting_location, int convoying_fleet_location) {
    // Paradox detection: Follow chain from starting_location to see if we eventually
    // find a support order supporting the convoying fleet
    // Returns 1 if paradox detected, 0 otherwise

    int visited_locations[MAX_LOCATIONS];
    int num_visited = 0;
    int current_location = starting_location;

    // Follow the chain for up to MAX_LOCATIONS steps
    while (num_visited < MAX_LOCATIONS) {
        // Check if we've visited this location before (cycle detection)
        for (int i = 0; i < num_visited; i++) {
            if (visited_locations[i] == current_location) {
                return 0;  // Cycle detected, no paradox
            }
        }
        visited_locations[num_visited++] = current_location;

        // Find unit at current location
        int unit_power = -1;
        Order* current_order = NULL;

        for (int p = 0; p < MAX_POWERS; p++) {
            Power* power = &game->powers[p];
            for (int u = 0; u < power->num_units; u++) {
                if (power->units[u].location == current_location) {
                    // Found unit, now find its order
                    for (int o = 0; o < power->num_orders; o++) {
                        if (power->orders[o].unit_location == current_location) {
                            current_order = &power->orders[o];
                            unit_power = p;
                            break;
                        }
                    }
                    break;
                }
            }
            if (current_order) break;
        }

        if (!current_order) {
            return 0;  // No unit or no order at this location
        }

        // Check if this is a support order for the convoying fleet
        if (current_order->type == ORDER_SUPPORT_HOLD || current_order->type == ORDER_SUPPORT_MOVE) {
            // Check if this support is for the fleet at convoying_fleet_location
            if (current_order->target_unit_location == convoying_fleet_location) {
                return 1;  // Paradox detected!
            }
        }

        // Only continue chain if order is Support or Convoy
        if (current_order->type != ORDER_SUPPORT_HOLD &&
            current_order->type != ORDER_SUPPORT_MOVE &&
            current_order->type != ORDER_CONVOY) {
            return 0;  // Chain ends
        }

        // Move to the next location in the chain
        if (current_order->type == ORDER_SUPPORT_MOVE || current_order->type == ORDER_CONVOY) {
            current_location = current_order->dest_location;
        } else {
            return 0;  // Hold support doesn't continue the chain
        }
    }

    return 0;  // No paradox found
}

// ============================================================================
// Phase Processing
// ============================================================================

typedef struct {
    int unit_power;
    int unit_idx;
    int from_location;
    int to_location;
    UnitType unit_type;
    int is_valid;
    int attack_strength;
    int defend_strength;
    int can_move;
    int support_cut;
    int is_convoyed;
    int convoy_disrupted;
    int order_idx;
} MoveAttempt;

typedef struct {
    int supporter_power;
    int supporter_location;
    int supported_location;
    int destination;
    int is_valid;
    int is_cut;
    int order_idx;
} SupportOrder;

void resolve_movement_phase(GameState* game) {
    game->num_dislodged = 0;
    game->num_combats = 0;

    for (int p = 0; p < MAX_POWERS; p++) {
        Power* power = &game->powers[p];
        for (int o = 0; o < power->num_orders; o++) {
            power->orders[o].result = RESULT_NONE;
        }
    }

    MoveAttempt attempts[MAX_POWERS * MAX_UNITS];
    int num_attempts = 0;

    SupportOrder supports[MAX_POWERS * MAX_UNITS];
    int num_supports = 0;

    // Step 1: Collect all move attempts from orders
    for (int p = 0; p < MAX_POWERS; p++) {
        Power* power = &game->powers[p];
        
        // Create default HOLD orders for units without explicit orders
        for (int u = 0; u < power->num_units; u++) {
            Unit* unit = &power->units[u];
            int has_order = 0;
            
            // Check if unit has an explicit order
            for (int o = 0; o < power->num_orders; o++) {
                if (power->orders[o].unit_location == unit->location) {
                    has_order = 1;
                    break;
                }
            }
            
            // If no order, create implicit HOLD
            if (!has_order && power->num_orders < MAX_UNITS) {
                Order hold_order;
                memset(&hold_order, 0, sizeof(Order));
                hold_order.type = ORDER_HOLD;
                hold_order.unit_location = unit->location;
                hold_order.unit_type = unit->type;
                hold_order.power_id = p;
                power->orders[power->num_orders++] = hold_order;
            }
        }
        
        // Process all orders for this power
        for (int o = 0; o < power->num_orders; o++) {
            Order* order = &power->orders[o];
            
            // Find the unit for this order
            int unit_idx = -1;
            for (int u = 0; u < power->num_units; u++) {
                if (power->units[u].location == order->unit_location) {
                    unit_idx = u;
                    break;
                }
            }
            
            if (unit_idx < 0) {
                continue;  // Order for non-existent unit
            }
            
            // Handle different order types
            if (order->type == ORDER_HOLD || order->type == ORDER_MOVE) {
                MoveAttempt* attempt = &attempts[num_attempts++];
                attempt->unit_power = p;
                attempt->unit_idx = unit_idx;
                attempt->from_location = order->unit_location;
                attempt->unit_type = order->unit_type;
                attempt->attack_strength = 1;  // Base strength (supports added later)
                attempt->defend_strength = 1;  // Base strength (supports added later)
                attempt->can_move = 0;         // Determined later
                attempt->support_cut = 0;      // Not supporting, so can't be cut
                attempt->is_convoyed = 0;      // Determined below for armies
                attempt->convoy_disrupted = 0; // Checked later
                attempt->order_idx = o;        // Track which order this came from

                if (order->type == ORDER_HOLD) {
                    attempt->to_location = -1;  // Holding
                    attempt->is_valid = 1;
                } else {  // ORDER_MOVE
                    attempt->to_location = order->target_location;
                    // Validate move
                    int valid = validate_order(game, p, order);
                    if (valid != 0) {
                        // Invalid move becomes a hold
                        attempt->to_location = -1;
                    }
                    attempt->is_valid = 1; // The attempt itself is now a valid action (either the original move or a hold)

                    // Check if this is a convoyed move (army moving to non-adjacent location)
                    if (attempt->to_location != -1 && order->unit_type == UNIT_ARMY) {
                        int is_adjacent = can_move(game->map, order->unit_type,
                                                   order->unit_location, order->target_location);
                        if (!is_adjacent) {
                            // Army move to non-adjacent location requires convoy
                            attempt->is_convoyed = 1;
                        }
                    }
                }
            }
            else if (order->type == ORDER_SUPPORT_HOLD || order->type == ORDER_SUPPORT_MOVE) {
                // Collect support orders
                SupportOrder* support = &supports[num_supports++];
                support->supporter_power = p;
                support->supporter_location = order->unit_location;
                support->supported_location = order->target_unit_location;
                support->destination = (order->type == ORDER_SUPPORT_MOVE) ?
                                      order->dest_location : order->target_unit_location;
                support->is_valid = (validate_order(game, p, order) == 0);
                support->is_cut = 0;  // Determined later
                support->order_idx = o;  // Track which order this came from

                // Also add this unit to move attempts as HOLD (supporting units hold their position)
                MoveAttempt* attempt = &attempts[num_attempts++];
                attempt->unit_power = p;
                attempt->unit_idx = unit_idx;
                attempt->from_location = order->unit_location;
                attempt->to_location = -1;  // Supporting units hold
                attempt->unit_type = order->unit_type;
                attempt->attack_strength = 1;
                attempt->defend_strength = 1;
                attempt->can_move = 0;
                attempt->is_valid = 1;
                attempt->support_cut = 0;  // Will be set if attacked
                attempt->is_convoyed = 0;  // Not a move
                attempt->convoy_disrupted = 0;
                attempt->order_idx = o;    // Track which order this came from
            }
            else if (order->type == ORDER_CONVOY) {
                // Convoying units also hold their position and can defend
                MoveAttempt* attempt = &attempts[num_attempts++];
                attempt->unit_power = p;
                attempt->unit_idx = unit_idx;
                attempt->from_location = order->unit_location;
                attempt->to_location = -1;  // Convoying units hold
                attempt->unit_type = order->unit_type;
                attempt->attack_strength = 1;
                attempt->defend_strength = 1;
                attempt->can_move = 0;
                attempt->is_valid = (validate_order(game, p, order) == 0);
                attempt->support_cut = 0;  // Not used for convoy, but initialize
                attempt->is_convoyed = 0;  // Not a move
                attempt->convoy_disrupted = 0;
                attempt->order_idx = o;    // Track which order this came from
            }
        }
    }

    // Step 2: Determine which supports are cut by attacks
    // A support is cut if the supporter is attacked by any unit (except the supported unit)
    // NOTE: Convoyed moves are checked later after convoy disruption (Step 6b)
    for (int i = 0; i < num_attempts; i++) {
        MoveAttempt* attacker = &attempts[i];

        if (!attacker->is_valid || attacker->to_location < 0) {
            continue;  // Not attacking
        }

        // Skip convoyed moves - they only cut support if convoy succeeds
        // (checked later after convoy disruption)
        if (attacker->is_convoyed) {
            continue;
        }

        // Check if this attack cuts any supports
        for (int s = 0; s < num_supports; s++) {
            SupportOrder* support = &supports[s];

            if (!support->is_valid || support->is_cut) {
                continue;  // Already invalid or cut
            }

            // Support is cut if attacker is moving to supporter's location
            // For split coasts, check parent location (e.g., SPA/SC and SPA/NC are same parent)
            int attacker_dest_parent = get_parent_location(game->map, attacker->to_location);
            int supporter_loc_parent = get_parent_location(game->map, support->supporter_location);

            if (attacker_dest_parent == supporter_loc_parent) {
                // Exception: attack from the unit being supported doesn't cut support
                int supported_loc_parent = get_parent_location(game->map, support->supported_location);
                int attacker_src_parent = get_parent_location(game->map, attacker->from_location);

                if (attacker_src_parent != supported_loc_parent) {
                    // Exception: own units don't cut support
                    if (attacker->unit_power != support->supporter_power) {
                        support->is_cut = 1;
                    }
                }
            }
        }
    }
    
    // Step 3: Calculate attack and defense strengths with valid supports
    for (int i = 0; i < num_attempts; i++) {
        MoveAttempt* attempt = &attempts[i];
        
        if (!attempt->is_valid) {
            continue;
        }
        
        // Calculate attack strength (if moving)
        if (attempt->to_location >= 0) {
            // Count valid supports for this move
            for (int s = 0; s < num_supports; s++) {
                SupportOrder* support = &supports[s];
                
                if (!support->is_valid || support->is_cut) {
                    continue;
                }
                
                // Check if this support applies to this move
                if (support->supported_location == attempt->from_location &&
                    support->destination == attempt->to_location) {
                    
                    // Additional check: supporter's power can't support dislodging own unit
                    // Find if there's own unit at destination
                    int own_unit_at_dest = 0;
                    for (int j = 0; j < num_attempts; j++) {
                        if (attempts[j].from_location == attempt->to_location &&
                            attempts[j].unit_power == support->supporter_power) {
                            own_unit_at_dest = 1;
                            break;
                        }
                    }
                    
                    if (!own_unit_at_dest) {
                        attempt->attack_strength++;
                    } else {
                        // Can't support dislodging own unit - mark support as invalid
                        support->is_valid = 0;
                    }
                }
            }
        }
        
        // Calculate defense strength (if holding or destination of attack)
        // Units get hold support
        for (int s = 0; s < num_supports; s++) {
            SupportOrder* support = &supports[s];
            
            if (!support->is_valid || support->is_cut) {
                continue;
            }
            
            // Hold support: supported unit is at its location
            if (support->supported_location == attempt->from_location &&
                support->destination == attempt->from_location) {
                attempt->defend_strength++;
            }
        }
    }
    
    // Step 4: Detect conflicts and determine outcomes using strength calculations
    for (int i = 0; i < num_attempts; i++) {
        MoveAttempt* attacker = &attempts[i];

        if (!attacker->is_valid || attacker->to_location < 0) {
            continue;  // Invalid order or holding
        }

        int destination = attacker->to_location;
        int source = attacker->from_location;
        
        // Find all competing moves to this destination
        int max_attack_strength = 0;
        int num_with_max_strength = 0;
        int head_to_head_opponent_idx = -1;
        
        for (int j = 0; j < num_attempts; j++) {
            MoveAttempt* other = &attempts[j];

            if (!other->is_valid || other->to_location != destination) {
                continue;  // Not attacking this destination
            }

            // Track max attack strength
            if (other->attack_strength > max_attack_strength) {
                max_attack_strength = other->attack_strength;
                num_with_max_strength = 1;
            } else if (other->attack_strength == max_attack_strength) {
                num_with_max_strength++;
            }
            
            // Check for head-to-head battle
            // For split coasts, check parent locations (e.g., BUL/SC <-> BUL/EC)
            int other_from_parent = get_parent_location(game->map, other->from_location);
            int other_to_parent = get_parent_location(game->map, other->to_location);
            int dest_parent = get_parent_location(game->map, destination);
            int src_parent = get_parent_location(game->map, source);

            if (other_from_parent == dest_parent && other_to_parent == src_parent) {
                head_to_head_opponent_idx = j;
            }
        }
        
        // Find defender at destination (if any)
        int defender_idx = -1;
        int defender_strength = 0;
        for (int j = 0; j < num_attempts; j++) {
            if (attempts[j].from_location == destination) {
                defender_idx = j;
                defender_strength = attempts[j].defend_strength;
                break;
            }
        }
        
        // Determine outcome based on strengths
        if (num_with_max_strength > 1) {
            // Multiple attackers with equal max strength → all bounce
            continue;  // can_move stays 0
        }
        
        // Single strongest attacker
        if (attacker->attack_strength == max_attack_strength) {
            if (defender_idx >= 0) {
                // There's a defender
                if (head_to_head_opponent_idx >= 0) {
                    // Head-to-head battle
                    MoveAttempt* opponent = &attempts[head_to_head_opponent_idx];
                    if (attacker->attack_strength > opponent->attack_strength) {
                        // We win - can move, they're dislodged
                        attacker->can_move = 1;
                        // Mark for dislodgement (tracked later)
                    } else {
                        // Equal or weaker - both bounce
                        continue;
                    }
                } else {
                    // Not head-to-head, defender holding or moving elsewhere
                    if (attacker->attack_strength > defender_strength) {
                        // Successful attack - defender dislodged
                        attacker->can_move = 1;
                        // Mark defender for dislodgement
                    } else {
                        // Attack bounced
                        continue;
                    }
                }
            } else {
                // No defender - move succeeds
                attacker->can_move = 1;
            }
        }
    }
    
    // Step 5: Handle circular movements and chains
    // Detect and resolve cycles like A→B, B→C, C→A
    for (int i = 0; i < num_attempts; i++) {
        MoveAttempt* attempt = &attempts[i];

        if (attempt->can_move || !attempt->is_valid || attempt->to_location < 0) {
            continue;  // Already resolved or not moving
        }

        // Try to find a cycle starting from this unit
        int visited[MAX_UNITS] = {0};
        int cycle_indices[MAX_UNITS];
        int cycle_len = 0;

        int current_idx = i;
        int found_cycle = 0;

        // Follow the chain of moves
        while (current_idx >= 0 && cycle_len < MAX_UNITS) {
            if (visited[current_idx]) {
                // Found a cycle - check if it starts at our original unit
                for (int c = 0; c < cycle_len; c++) {
                    if (cycle_indices[c] == current_idx) {
                        found_cycle = 1;
                        // Trim cycle to only include units in the actual cycle
                        for (int k = c; k < cycle_len; k++) {
                            cycle_indices[k - c] = cycle_indices[k];
                        }
                        cycle_len -= c;
                        break;
                    }
                }
                break;
            }

            visited[current_idx] = 1;
            cycle_indices[cycle_len++] = current_idx;

            // Find unit at destination
            int next_idx = -1;
            int dest = attempts[current_idx].to_location;
            for (int j = 0; j < num_attempts; j++) {
                if (attempts[j].from_location == dest && attempts[j].to_location >= 0) {
                    next_idx = j;
                    break;
                }
            }

            current_idx = next_idx;
        }

        // If we found a cycle, check if all units in cycle can move
        if (found_cycle && cycle_len >= 2) {
            int can_resolve_cycle = 1;

            // Check if any external unit is attacking into the cycle
            for (int c = 0; c < cycle_len; c++) {
                int cycle_dest = attempts[cycle_indices[c]].to_location;
                int cycle_strength = attempts[cycle_indices[c]].attack_strength;

                // Check for external attackers
                for (int j = 0; j < num_attempts; j++) {
                    if (attempts[j].to_location == cycle_dest && attempts[j].is_valid) {
                        // Is this attacker part of the cycle?
                        int in_cycle = 0;
                        for (int k = 0; k < cycle_len; k++) {
                            if (j == cycle_indices[k]) {
                                in_cycle = 1;
                                break;
                            }
                        }

                        if (!in_cycle && attempts[j].attack_strength >= cycle_strength) {
                            // External unit blocks the cycle
                            can_resolve_cycle = 0;
                            break;
                        }
                    }
                }

                if (!can_resolve_cycle) break;
            }

            // If cycle can resolve, mark all units in cycle as can_move
            if (can_resolve_cycle) {
                for (int c = 0; c < cycle_len; c++) {
                    attempts[cycle_indices[c]].can_move = 1;
                }
            }
        }
    }

    // Step 5b: Iteratively resolve remaining moves where destination is being vacated
    int changed = 1;
    int iterations = 0;
    while (changed && iterations < 20) {
        changed = 0;
        iterations++;

        for (int i = 0; i < num_attempts; i++) {
            MoveAttempt* attempt = &attempts[i];

            if (attempt->can_move || !attempt->is_valid || attempt->to_location < 0) {
                continue;  // Already resolved or not moving
            }

            int destination = attempt->to_location;

            // Check if destination unit is moving away successfully
            int dest_unit_moving = 0;
            for (int j = 0; j < num_attempts; j++) {
                if (attempts[j].from_location == destination &&
                    attempts[j].to_location >= 0 &&
                    attempts[j].can_move) {
                    dest_unit_moving = 1;
                    break;
                }
            }

            if (!dest_unit_moving) {
                continue;  // Destination not being vacated
            }

            // Check if we're the only/strongest attacker to this destination
            int is_strongest = 1;
            int max_str = attempt->attack_strength;
            int num_at_max = 1;

            for (int j = 0; j < num_attempts; j++) {
                if (i == j) continue;
                if (!attempts[j].is_valid || attempts[j].to_location != destination) {
                    continue;
                }

                if (attempts[j].attack_strength > max_str) {
                    is_strongest = 0;
                    break;
                } else if (attempts[j].attack_strength == max_str) {
                    num_at_max++;
                }
            }

            // Can move if strongest attacker
            if (is_strongest && num_at_max == 1) {
                attempt->can_move = 1;
                changed = 1;
            }
        }
    }
    
    // Step 6: Determine and record dislodgements
    for (int i = 0; i < num_attempts; i++) {
        MoveAttempt* attacker = &attempts[i];
        
        if (!attacker->can_move || attacker->to_location < 0) {
            continue;  // Not moving
        }
        
        // Check if we're dislodging a unit
        for (int j = 0; j < num_attempts; j++) {
            MoveAttempt* defender = &attempts[j];
            
            if (defender->from_location == attacker->to_location) {
                // There's a unit at our destination
                // It's dislodged if we have strength to dislodge it
                if (attacker->attack_strength > defender->defend_strength) {
                    // Record dislodgement
                    DislodgedUnit* dislodged = &game->dislodged[game->num_dislodged++];
                    dislodged->type = defender->unit_type;
                    dislodged->power_id = defender->unit_power;
                    dislodged->from_location = defender->from_location;
                    dislodged->dislodged_by_location = attacker->from_location;
                    dislodged->attacker_used_convoy = attacker->is_convoyed ? 1 : 0;
                    dislodged->num_possible_retreats = 0;  // Calculate later in retreat phase

                    // Mark defender as not able to stay
                    // We'll remove the unit when applying moves
                }
                break;
            }
        }
    }
    
    // Step 6b: Check convoy disruption and handle support cutting for convoyed moves
    // For convoyed moves:
    // - If convoy is disrupted (fleet dislodged), move fails and support is NOT cut
    // - If convoy is valid, check if move succeeds and cut support if it attacks
    for (int i = 0; i < num_attempts; i++) {
        MoveAttempt* attempt = &attempts[i];

        if (!attempt->is_convoyed || attempt->to_location < 0) {
            continue;  // Not a convoyed move
        }

        // This is a convoyed move - check if convoy is still valid after dislodgements
        // Build list of non-dislodged convoying fleets
        int convoying_fleets[MAX_LOCATIONS];
        int num_fleets = 0;
        int dislodged_fleet_locs[MAX_LOCATIONS];
        int num_dislodged_fleets = 0;

        // Get all fleets offering to convoy this move
        for (int p = 0; p < MAX_POWERS; p++) {
            Power* power = &game->powers[p];
            for (int o = 0; o < power->num_orders; o++) {
                Order* order = &power->orders[o];

                if (order->type == ORDER_CONVOY &&
                    order->target_unit_location == attempt->from_location &&
                    order->dest_location == attempt->to_location) {

                    int fleet_loc = order->unit_location;

                    // Check if this fleet is dislodged
                    int is_dislodged = 0;
                    for (int d = 0; d < game->num_dislodged; d++) {
                        if (game->dislodged[d].from_location == fleet_loc) {
                            is_dislodged = 1;
                            dislodged_fleet_locs[num_dislodged_fleets++] = fleet_loc;
                            break;
                        }
                    }

                    // Only include non-dislodged fleets in water
                    if (!is_dislodged && can_fleet_convoy(game->map, fleet_loc)) {
                        convoying_fleets[num_fleets++] = fleet_loc;
                    }
                }
            }
        }

        // Check if convoy path still exists with remaining fleets
        int convoy_valid = (num_fleets > 0 &&
                           find_convoy_path(game->map, attempt->from_location,
                                          attempt->to_location,
                                          convoying_fleets, num_fleets));

        if (!convoy_valid && num_dislodged_fleets > 0) {
            // Convoy appears disrupted - check for paradox
            // Paradox: destination unit supports a dislodged convoying fleet
            int is_paradox = 0;
            for (int df = 0; df < num_dislodged_fleets; df++) {
                if (detect_paradox(game, attempt->to_location, dislodged_fleet_locs[df])) {
                    is_paradox = 1;
                    break;
                }
            }

            if (!is_paradox) {
                // Not a paradox - convoy is disrupted, move fails and does NOT cut support
                attempt->convoy_disrupted = 1;
                attempt->can_move = 0;
            }
            // If paradox, don't mark as disrupted - convoy succeeds despite dislodged fleet
        } else if (!convoy_valid) {
            // No valid path and no dislodged fleets (invalid from start)
            attempt->convoy_disrupted = 1;
            attempt->can_move = 0;
        }

        // If convoy is valid (or paradox), check if it cuts any supports
        if (!attempt->convoy_disrupted) {
            // Convoy is valid - check if it cuts any supports
            // (convoyed moves cut support just like normal moves, if convoy succeeds)
            for (int s = 0; s < num_supports; s++) {
                SupportOrder* support = &supports[s];

                if (!support->is_valid || support->is_cut) {
                    continue;  // Already invalid or cut
                }

                // Support is cut if attacker is moving to supporter's location
                // For split coasts, check parent location
                int convoy_dest_parent = get_parent_location(game->map, attempt->to_location);
                int supporter_parent = get_parent_location(game->map, support->supporter_location);

                if (convoy_dest_parent == supporter_parent) {
                    // Exception: attack from the unit being supported doesn't cut support
                    int supported_parent = get_parent_location(game->map, support->supported_location);
                    int convoy_src_parent = get_parent_location(game->map, attempt->from_location);

                    if (convoy_src_parent != supported_parent) {
                        // Exception: own units don't cut support
                        if (attempt->unit_power != support->supporter_power) {
                            support->is_cut = 1;
                        }
                    }
                }
            }
        }
    }

    // Step 6c: Resolve valid convoyed moves
    // Now that we know which convoys are disrupted, resolve the valid ones
    for (int i = 0; i < num_attempts; i++) {
        MoveAttempt* attempt = &attempts[i];

        if (!attempt->is_convoyed || attempt->to_location < 0) {
            continue;  // Not a convoyed move
        }

        if (attempt->convoy_disrupted) {
            continue;  // Convoy failed, already marked
        }

        // Convoy is valid - check if move can succeed
        int destination = attempt->to_location;

        // Check if destination is vacant or being vacated
        int dest_vacant = 1;
        int dest_unit_moving = 0;

        for (int j = 0; j < num_attempts; j++) {
            if (attempts[j].from_location == destination) {
                dest_vacant = 0;
                if (attempts[j].to_location >= 0 && attempts[j].can_move) {
                    dest_unit_moving = 1;
                }
                break;
            }
        }

        if (!dest_vacant && !dest_unit_moving) {
            continue;  // Destination occupied and not moving - convoy bounces
        }

        // Check if we're the strongest attacker (among other convoyed moves and already-resolved moves)
        int is_strongest = 1;
        int max_str = attempt->attack_strength;

        for (int j = 0; j < num_attempts; j++) {
            if (i == j) continue;
            if (!attempts[j].is_valid || attempts[j].to_location != destination) {
                continue;
            }

            // Check other convoyed moves
            if (attempts[j].is_convoyed && !attempts[j].convoy_disrupted) {
                if (attempts[j].attack_strength >= max_str) {
                    is_strongest = 0;
                    break;
                }
            }
            // Check already-resolved non-convoyed moves
            else if (!attempts[j].is_convoyed && attempts[j].can_move) {
                // There's already a stronger move succeeding
                is_strongest = 0;
                break;
            }
        }

        if (is_strongest) {
            attempt->can_move = 1;
        }
    }

    // Step 7: Apply successful moves and remove dislodged units
    for (int i = 0; i < num_attempts; i++) {
        MoveAttempt* attempt = &attempts[i];

        if (!attempt->can_move || attempt->to_location < 0) {
            continue;  // Stay in place
        }
        
        // Check if this unit was dislodged
        int was_dislodged = 0;
        for (int d = 0; d < game->num_dislodged; d++) {
            if (game->dislodged[d].power_id == attempt->unit_power &&
                game->dislodged[d].from_location == attempt->from_location) {
                was_dislodged = 1;
                break;
            }
        }
        
        if (was_dislodged) {
            // Don't apply move for dislodged unit - it will be removed/retreated
            continue;
        }
        
        // Move the unit
        Power* power = &game->powers[attempt->unit_power];
        Unit* unit = &power->units[attempt->unit_idx];
        unit->location = attempt->to_location;
    }
    
    for (int d = 0; d < game->num_dislodged; d++) {
        DislodgedUnit* dislodged = &game->dislodged[d];
        Power* power = &game->powers[dislodged->power_id];
        int has_void_order = 0;

        // Check if this unit had a VOID order - if so, don't remove it
        for (int o = 0; o < power->num_orders; o++) {
            Order* order = &power->orders[o];
            if (attempts[o].from_location == dislodged->from_location && !attempts[o].is_valid) {
                has_void_order = 1;
                break;
            }
        }

        // Skip removal if unit had VOID order (stays at original location)
        if (has_void_order) {
            continue;
        }

        // Remove unit from power's unit list
        for (int u = 0; u < power->num_units; u++) {
            if (power->units[u].location == dislodged->from_location &&
                power->units[u].type == dislodged->type) {
                // Remove by shifting remaining units
                for (int k = u + 1; k < power->num_units; k++) {
                    power->units[k - 1] = power->units[k];
                }
                power->num_units--;
                break;
            }
        }
    }

    // Final check for multiple successful moves to the same destination
    for (int i = 0; i < num_attempts; i++) {
        if (attempts[i].can_move && attempts[i].to_location != -1) {
            for (int j = i + 1; j < num_attempts; j++) {
                if (attempts[j].can_move && attempts[j].to_location == attempts[i].to_location) {
                    // Two units moved to the same location. Both should bounce.
                    attempts[i].can_move = 0;
                    attempts[j].can_move = 0;
                }
            }
        }
    }

    // Set result codes for all orders
    for (int i = 0; i < num_attempts; i++) {
        MoveAttempt* attempt = &attempts[i];

        // Skip if this is an implicit hold (no corresponding order)
        if (attempt->order_idx < 0) {
            continue;
        }

        Power* power = &game->powers[attempt->unit_power];
        Order* order = &power->orders[attempt->order_idx];

        // Check if unit was dislodged
        int was_dislodged = 0;
        for (int d = 0; d < game->num_dislodged; d++) {
            if (game->dislodged[d].power_id == attempt->unit_power &&
                game->dislodged[d].from_location == attempt->from_location) {
                was_dislodged = 1;
                break;
            }
        }

        if (was_dislodged) {
            order->result = RESULT_DISLODGED;
        } else if (order->type == ORDER_MOVE) {
            if (attempt->can_move && attempt->to_location != -1) {
                order->result = RESULT_SUCCESS;
            } else if (attempt->to_location == -1) {
                // Invalid move that was converted to hold
                order->result = RESULT_VOID;
            } else {
                // Move failed - bounced
                order->result = RESULT_BOUNCE;
            }
        } else if (order->type == ORDER_HOLD) {
            // Hold orders that weren't dislodged succeeded
            order->result = was_dislodged ? RESULT_DISLODGED : RESULT_SUCCESS;
        } else if (order->type == ORDER_CONVOY) {
            // Convoy orders: check if valid
            if (!attempt->is_valid) {
                order->result = RESULT_VOID;
            } else {
                order->result = RESULT_SUCCESS;
            }
        }
    }

    // Set results for support orders
    for (int i = 0; i < num_supports; i++) {
        SupportOrder* support = &supports[i];
        Power* power = &game->powers[support->supporter_power];
        Order* order = &power->orders[support->order_idx];

        // Check if supporter was dislodged
        int was_dislodged = 0;
        for (int d = 0; d < game->num_dislodged; d++) {
            if (game->dislodged[d].power_id == support->supporter_power &&
                game->dislodged[d].from_location == support->supporter_location) {
                was_dislodged = 1;
                break;
            }
        }

        if (was_dislodged) {
            order->result = RESULT_DISLODGED;
        } else if (!support->is_valid) {
            order->result = RESULT_VOID;
        } else if (support->is_cut) {
            order->result = RESULT_CUT;
        } else {
            order->result = RESULT_SUCCESS;
        }
    }

    // Save results to persistent storage before they get cleared
    for (int p = 0; p < MAX_POWERS; p++) {
        Power* power = &game->powers[p];
        game->last_num_orders[p] = power->num_orders;
        for (int o = 0; o < power->num_orders; o++) {
            game->last_results[p][o] = power->orders[o].result;
        }
    }

    // Calculate retreat destinations for dislodged units
    calculate_retreat_destinations(game);
}

void calculate_retreat_destinations(GameState* game) {
    // Calculate valid retreat destinations for all dislodged units
    // Called after movement resolution

    for (int d = 0; d < game->num_dislodged; d++) {
        DislodgedUnit* dislodged = &game->dislodged[d];
        Power* power = &game->powers[dislodged->power_id];

        // Add this unit to power's retreat list
        DislodgedUnit* retreat = &power->retreats[power->num_retreats++];
        *retreat = *dislodged;  // Copy dislodged unit info
        retreat->num_possible_retreats = 0;

        int from_loc = dislodged->from_location;
        int attacker_loc = dislodged->dislodged_by_location;

        // Find all adjacent locations where unit can retreat
        Location* loc = &game->map->locations[from_loc];

        for (int a = 0; a < loc->num_adjacent; a++) {
            int adj_loc = loc->adjacencies[a];

            // Check if unit can move there (based on unit type and terrain)
            if (!can_move(game->map, dislodged->type, from_loc, adj_loc)) {
                continue;
            }

            // Cannot retreat to attacker's origin
            // UNLESS the attacker arrived via convoy (DATC 6.H.11-13)
            if (adj_loc == attacker_loc && !dislodged->attacker_used_convoy) {
                continue;
            }

            // Cannot retreat to a location that had combat (contested)
            int had_combat = 0;
            for (int c = 0; c < game->num_combats; c++) {
                if (game->combats[c].location == adj_loc) {
                    had_combat = 1;
                    break;
                }
            }

            if (had_combat) {
                continue;  // Skip contested location
            }

            // Valid retreat destination
            retreat->possible_retreats[retreat->num_possible_retreats].location = adj_loc;
            retreat->num_possible_retreats++;
        }
    }
}

void resolve_retreat_phase(GameState* game) {
    // Process retreat orders
    // 1. Parse retreat orders from power->orders[]
    // 2. Check for conflicts (multiple units retreating to same location)
    // 3. Move units or disband them

    // Initialize all order results to NONE
    for (int p = 0; p < MAX_POWERS; p++) {
        Power* power = &game->powers[p];
        for (int o = 0; o < power->num_orders; o++) {
            power->orders[o].result = RESULT_NONE;
        }
    }

    int retreat_destinations[MAX_UNITS];  // Where each unit is retreating to (-1 = disband)
    int retreat_power[MAX_UNITS];
    int num_retreat_orders = 0;

    // Process retreat orders for each power
    for (int p = 0; p < MAX_POWERS; p++) {
        Power* power = &game->powers[p];

        if (power->num_retreats == 0) {
            continue;  // No dislodged units
        }

        // If no orders given, disband all retreating units
        if (power->num_orders == 0) {
            for (int r = 0; r < power->num_retreats; r++) {
                // Auto-disband
                retreat_destinations[num_retreat_orders] = -1;
                retreat_power[num_retreat_orders] = p;
                num_retreat_orders++;
            }
            continue;
        }

        // Parse retreat orders
        for (int r = 0; r < power->num_retreats; r++) {
            DislodgedUnit* dislodged = &power->retreats[r];
            int found_order = 0;

            // Find matching retreat order
            for (int o = 0; o < power->num_orders; o++) {
                Order* order = &power->orders[o];

                if (order->type == ORDER_RETREAT &&
                    order->unit_location == dislodged->from_location) {
                    // Found retreat order for this unit

                    // Check if destination is valid
                    int valid_dest = 0;
                    for (int d = 0; d < dislodged->num_possible_retreats; d++) {
                        if (dislodged->possible_retreats[d].location == order->target_location) {
                            valid_dest = 1;
                            break;
                        }
                    }

                    // Check if destination is occupied by another unit
                    int occupied = 0;
                    if (valid_dest) {
                        for (int check_p = 0; check_p < MAX_POWERS; check_p++) {
                            Power* check_power = &game->powers[check_p];
                            for (int u = 0; u < check_power->num_units; u++) {
                                if (check_power->units[u].location == order->target_location) {
                                    occupied = 1;
                                    break;
                                }
                            }
                            if (occupied) break;
                        }
                    }

                    if (valid_dest && !occupied) {
                        retreat_destinations[num_retreat_orders] = order->target_location;
                        retreat_power[num_retreat_orders] = p;
                        order->result = RESULT_SUCCESS;  // Tentative success (may change if conflict)
                        num_retreat_orders++;
                    } else {
                        // Invalid destination or occupied - disband
                        retreat_destinations[num_retreat_orders] = -1;
                        retreat_power[num_retreat_orders] = p;
                        order->result = RESULT_VOID;  // Invalid retreat
                        num_retreat_orders++;
                    }
                    found_order = 1;
                    break;
                }

                if (order->type == ORDER_DISBAND &&
                    order->unit_location == dislodged->from_location) {
                    // Explicit disband order
                    retreat_destinations[num_retreat_orders] = -1;
                    retreat_power[num_retreat_orders] = p;
                    order->result = RESULT_SUCCESS;  // Disband succeeded
                    num_retreat_orders++;
                    found_order = 1;
                    break;
                }
            }

            if (!found_order) {
                // No order for this unit - auto-disband
                retreat_destinations[num_retreat_orders] = -1;
                retreat_power[num_retreat_orders] = p;
                // No order to set result for (implicit disband)
                num_retreat_orders++;
            }
        }
    }

    // Check for conflicts: multiple units retreating to same location
    // Use a conflict marker array to avoid corrupting destinations during detection
    int has_conflict[MAX_UNITS] = {0};
    for (int i = 0; i < num_retreat_orders; i++) {
        if (retreat_destinations[i] == -1) {
            continue;  // Already disbanding
        }

        for (int j = i + 1; j < num_retreat_orders; j++) {
            if (retreat_destinations[j] == -1) {
                continue;  // Already disbanding
            }

            if (retreat_destinations[i] == retreat_destinations[j]) {
                // Conflict! Mark both units to disband
                has_conflict[i] = 1;
                has_conflict[j] = 1;
            }
        }
    }

    // Apply conflicts: set all conflicting retreats to disband
    // And update results to BOUNCE
    int order_idx = 0;
    for (int p = 0; p < MAX_POWERS; p++) {
        Power* power = &game->powers[p];
        for (int o = 0; o < power->num_orders; o++) {
            Order* order = &power->orders[o];
            if (order->type == ORDER_RETREAT || order->type == ORDER_DISBAND) {
                if (order_idx < num_retreat_orders && has_conflict[order_idx]) {
                    retreat_destinations[order_idx] = -1;
                    if (order->type == ORDER_RETREAT) {
                        order->result = RESULT_BOUNCE;  // Conflicted retreat
                    }
                }
                order_idx++;
            }
        }
    }

    // Apply retreats
    int retreat_idx = 0;
    for (int p = 0; p < MAX_POWERS; p++) {
        Power* power = &game->powers[p];

        for (int r = 0; r < power->num_retreats; r++) {
            DislodgedUnit* dislodged = &power->retreats[r];

            if (retreat_idx < num_retreat_orders && retreat_destinations[retreat_idx] != -1) {
                // Unit successfully retreats
                Unit new_unit;
                new_unit.type = dislodged->type;
                new_unit.location = retreat_destinations[retreat_idx];
                new_unit.power_id = p;
                new_unit.can_retreat = 0;

                // Add unit to power's unit list
                power->units[power->num_units++] = new_unit;
            }
            // else: unit disbands (do nothing, already removed)

            retreat_idx++;
        }

        // Clear retreat list
        power->num_retreats = 0;
    }

    // Clear dislodged units
    game->num_dislodged = 0;

    // Save results to persistent storage
    for (int p = 0; p < MAX_POWERS; p++) {
        Power* power = &game->powers[p];
        game->last_num_orders[p] = power->num_orders;
        for (int o = 0; o < power->num_orders; o++) {
            game->last_results[p][o] = power->orders[o].result;
        }
    }
}

void resolve_adjustment_phase(GameState* game) {
    // Initialize all order results to NONE
    for (int p = 0; p < MAX_POWERS; p++) {
        Power* power = &game->powers[p];
        for (int o = 0; o < power->num_orders; o++) {
            power->orders[o].result = RESULT_NONE;
        }
    }

    // Process build and disband orders for each power
    for (int p = 0; p < MAX_POWERS; p++) {
        Power* power = &game->powers[p];
        int diff = power->num_units - power->num_centers;

        // Process each order
        for (int o = 0; o < power->num_orders; o++) {
            Order* order = &power->orders[o];

            // Process BUILD orders
            if (order->type == ORDER_BUILD) {
                // Check if we can still build more units
                if (diff < 0) {
                    // Validate build location
                    int is_valid_build = 0;

                    // Check if location is a supply center owned by this power
                    Location* loc = &game->map->locations[order->unit_location];
                    if (loc->has_supply_center && loc->owner_power == p) {
                        // Check if location is empty
                        int location_empty = 1;
                        for (int u = 0; u < power->num_units; u++) {
                            if (power->units[u].location == order->unit_location) {
                                location_empty = 0;
                                break;
                            }
                        }

                        // Check if it's a home center (standard Diplomacy rule)
                        if (location_empty && loc->is_home_center == p) {
                            is_valid_build = 1;
                        }
                    }

                    // Execute build if valid
                    if (is_valid_build && power->num_units < MAX_UNITS) {
                        Unit new_unit;
                        new_unit.type = order->unit_type;
                        new_unit.location = order->unit_location;
                        new_unit.power_id = p;
                        new_unit.can_retreat = 0;

                        power->units[power->num_units++] = new_unit;
                        diff++;
                        order->result = RESULT_SUCCESS;
                    } else {
                        order->result = RESULT_VOID;  // Invalid build
                    }
                } else {
                    // Build limit reached - mark as VOID
                    order->result = RESULT_VOID;
                }
            }
            // Process DISBAND orders
            else if (order->type == ORDER_DISBAND) {
                // In standard mode: only allow if more units than centers
                // In welfare mode: always allow voluntary disbands
                if (diff > 0 || game->welfare_mode) {
                    // Find and remove the unit
                    int unit_found = 0;
                    for (int u = 0; u < power->num_units; u++) {
                        if (power->units[u].location == order->unit_location) {
                            // Remove unit by shifting array
                            for (int i = u; i < power->num_units - 1; i++) {
                                power->units[i] = power->units[i + 1];
                            }
                            power->num_units--;
                            diff--;
                            unit_found = 1;
                            break;
                        }
                    }
                    order->result = unit_found ? RESULT_SUCCESS : RESULT_VOID;
                } else {
                    order->result = RESULT_VOID;  // Not allowed to disband
                }
            }
        }

        // Civil disorder: auto-disband if still over supply limit
        while (diff > 0 && power->num_units > 0) {
            // Find unit furthest from home centers (simplified: just remove last unit)
            // TODO: Implement proper distance-based selection
            power->num_units--;
            diff--;
        }

        // Update adjustment count
        power->adjustment = power->num_centers - power->num_units;
    }

    // Save results to persistent storage BEFORE clearing orders
    for (int p = 0; p < MAX_POWERS; p++) {
        Power* power = &game->powers[p];
        game->last_num_orders[p] = power->num_orders;
        for (int o = 0; o < power->num_orders; o++) {
            game->last_results[p][o] = power->orders[o].result;
        }
    }

    // Clear orders after saving results
    for (int p = 0; p < MAX_POWERS; p++) {
        game->powers[p].num_orders = 0;
    }

    // Welfare Diplomacy: Calculate welfare points after adjustments
    if (game->welfare_mode) {
        for (int p = 0; p < MAX_POWERS; p++) {
            Power* power = &game->powers[p];
            int welfare_gain = power->num_centers - power->num_units;
            if (welfare_gain > 0) {
                power->welfare_points += welfare_gain;
            }
        }
    }
}

void advance_phase(GameState* game) {
    // Advance to next phase
    switch (game->phase) {
        case PHASE_SPRING_MOVEMENT:
            // Check if any units were dislodged
            if (game->num_dislodged > 0) {
                game->phase = PHASE_SPRING_RETREAT;
            } else {
                game->phase = PHASE_FALL_MOVEMENT;
            }
            break;
        case PHASE_SPRING_RETREAT:
            game->phase = PHASE_FALL_MOVEMENT;
            break;
        case PHASE_FALL_MOVEMENT:
            // Check if any units were dislodged
            if (game->num_dislodged > 0) {
                game->phase = PHASE_FALL_RETREAT;
            } else {
                game->phase = PHASE_WINTER_ADJUSTMENT;
            }
            break;
        case PHASE_FALL_RETREAT:
            game->phase = PHASE_WINTER_ADJUSTMENT;
            break;
        case PHASE_WINTER_ADJUSTMENT:
            // Move to next year
            game->year++;
            game->phase = PHASE_SPRING_MOVEMENT;

            // Check if game should end
            // max_years means "play for N years starting from 1901"
            // e.g., max_years=10 means play years 1901-1910, end when year becomes 1911
            if (game->year > 1900 + game->max_years) {
                game->phase = PHASE_COMPLETED;
                game->is_game_over = 1;
            }
            break;
        case PHASE_COMPLETED:
            // Game is over, no more phases
            break;
    }

    // Clear orders for new phase
    for (int p = 0; p < MAX_POWERS; p++) {
        game->powers[p].num_orders = 0;
    }
}

// ============================================================================
// Welfare Diplomacy Specific
// ============================================================================

// ============================================================================
// PufferLib Integration Functions
// ============================================================================

void c_init(Env* env) {
    // Allocate and initialize game state
    env->game = (GameState*)calloc(1, sizeof(GameState));
    if (!env->game) {
        fprintf(stderr, "Failed to allocate GameState\n");
        return;
    }

    // Allocate and initialize map
    Map* map = (Map*)calloc(1, sizeof(Map));
    if (!map) {
        fprintf(stderr, "Failed to allocate Map\n");
        free(env->game);
        return;
    }

    init_standard_map(map);
    init_game(env->game, map, 1, 10);  // Default: Welfare mode, 10 year limit

    // Initialize log
    memset(&env->log, 0, sizeof(Log));

    // Initialize reward bookkeeping
    for (int i = 0; i < MAX_POWERS; i++) {
        env->last_welfare[i] = 0;
    }
}

void c_reset(Env* env) {
    if (env->game) {
        reset_game(env->game);
    }
    // Reset rewards and terminals
    if (env->rewards) {
        for (int i = 0; i < MAX_POWERS; i++) env->rewards[i] = 0.0f;
    }
    if (env->terminals) {
        for (int i = 0; i < MAX_POWERS; i++) env->terminals[i] = 0;
    }
    // Initialize last welfare snapshot
    for (int i = 0; i < MAX_POWERS; i++) env->last_welfare[i] = env->game->powers[i].welfare_points;

    // Encode observations once on reset
    if (env->observations) {
        float* obs = (float*)env->observations;
        int stride = 175;
        for (int agent = 0; agent < MAX_POWERS; agent++) {
            float* base = obs + agent * stride;
            // Board ownership
            for (int i = 0; i < env->game->map->num_locations; i++) {
                base[i] = (float)env->game->map->locations[i].owner_power;
            }
            // Unit type at each location (0 none, 1 army, 2 fleet)
            int offset = 75;
            for (int i = 0; i < env->game->map->num_locations; i++) {
                int owner = get_unit_at_location(env->game, i);
                if (owner >= 0) {
                    UnitType t = UNIT_NONE;
                    for (int u = 0; u < env->game->powers[owner].num_units; u++) {
                        if (env->game->powers[owner].units[u].location == i) {
                            t = env->game->powers[owner].units[u].type;
                            break;
                        }
                    }
                    base[offset + i] = (float)t;
                } else {
                    base[offset + i] = 0.0f;
                }
            }
            // Centers per power
            offset += 75;
            for (int p = 0; p < MAX_POWERS; p++) {
                base[offset + p] = (float)env->game->powers[p].num_centers;
            }
            // Units per power
            offset += 7;
            for (int p = 0; p < MAX_POWERS; p++) {
                base[offset + p] = (float)env->game->powers[p].num_units;
            }
            // Welfare per power
            offset += 7;
            for (int p = 0; p < MAX_POWERS; p++) {
                base[offset + p] = (float)env->game->powers[p].welfare_points;
            }
            // Phase and year
            offset += 7;
            base[offset + 0] = (float)env->game->phase;
            base[offset + 1] = (float)env->game->year;
        }
    }
}

void c_step(Env* env) {
    if (!env->game || env->game->is_game_over) {
        return;
    }

    GameState* game = env->game;

    // Process current phase
    switch (game->phase) {
        case PHASE_SPRING_MOVEMENT:
        case PHASE_FALL_MOVEMENT:
            resolve_movement_phase(game);
            break;
        case PHASE_SPRING_RETREAT:
        case PHASE_FALL_RETREAT:
            resolve_retreat_phase(game);
            break;
        case PHASE_WINTER_ADJUSTMENT:
            resolve_adjustment_phase(game);
            break;
        case PHASE_COMPLETED:
            break;
    }

    // Advance to next phase
    advance_phase(game);

    // TODO: Process actions from env->actions (parse orders from action space)
    // Update observations (encode without resetting)
    if (env->observations) {
        float* obs = (float*)env->observations;
        int stride = 175;
        for (int agent = 0; agent < MAX_POWERS; agent++) {
            float* base = obs + agent * stride;
            for (int i = 0; i < env->game->map->num_locations; i++) {
                base[i] = (float)env->game->map->locations[i].owner_power;
            }
            int offset = 75;
            for (int i = 0; i < env->game->map->num_locations; i++) {
                int owner = get_unit_at_location(env->game, i);
                if (owner >= 0) {
                    UnitType t = UNIT_NONE;
                    for (int u = 0; u < env->game->powers[owner].num_units; u++) {
                        if (env->game->powers[owner].units[u].location == i) {
                            t = env->game->powers[owner].units[u].type;
                            break;
                        }
                    }
                    base[offset + i] = (float)t;
                } else {
                    base[offset + i] = 0.0f;
                }
            }
            offset += 75;
            for (int p = 0; p < MAX_POWERS; p++) {
                base[offset + p] = (float)env->game->powers[p].num_centers;
            }
            offset += 7;
            for (int p = 0; p < MAX_POWERS; p++) {
                base[offset + p] = (float)env->game->powers[p].num_units;
            }
            offset += 7;
            for (int p = 0; p < MAX_POWERS; p++) {
                base[offset + p] = (float)env->game->powers[p].welfare_points;
            }
            offset += 7;
            base[offset + 0] = (float)env->game->phase;
            base[offset + 1] = (float)env->game->year;
        }
    }

    // Calculate rewards at end of adjustment phase based on welfare deltas
    if (game->phase == PHASE_WINTER_ADJUSTMENT) {
        for (int i = 0; i < MAX_POWERS; i++) {
            int current = game->powers[i].welfare_points;
            int delta = current - env->last_welfare[i];
            if (env->rewards) env->rewards[i] = (float)delta;
            env->last_welfare[i] = current;
        }
    } else {
        if (env->rewards) {
            for (int i = 0; i < MAX_POWERS; i++) env->rewards[i] = 0.0f;
        }
    }

    // Set terminals if game over
    if (game->is_game_over) {
        for (int i = 0; i < MAX_POWERS; i++) {
            env->terminals[i] = 1;
        }
    }
}

void c_render(Env* env) {
    // TODO: Implement basic text rendering of game state
    if (!env->game) {
        return;
    }

    GameState* game = env->game;
    printf("Year: %d, Phase: %s\n", game->year, phase_to_string(game->phase));

    for (int p = 0; p < MAX_POWERS; p++) {
        Power* power = &game->powers[p];
        printf("%s: Centers=%d, Units=%d, Welfare=%d\n",
               power->name, power->num_centers, power->num_units, power->welfare_points);
    }
}

void c_close(Env* env) {
    if (env->game) {
        if (env->game->map) {
            free_map(env->game->map);
            free(env->game->map);
        }
        free_game(env->game);
        free(env->game);
        env->game = NULL;
    }
}

void c_configure(Env* env, int welfare_mode, int max_years) {
    if (!env || !env->game) {
        return;
    }
    // Re-init game in-place preserving map pointer
    Map* map = env->game->map;
    init_game(env->game, map, welfare_mode, max_years);
}
