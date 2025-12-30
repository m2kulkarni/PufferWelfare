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

    // First, try to find exact match and check if it's reachable for FLEETS
    // This function is only called for fleet moves, so use fleet adjacency cache
    int exact = find_location_by_name(map, dest_name);
    if (exact != -1) {
        // Check if exact match is reachable for fleets using adjacency cache
        if (map->adjacency_cache[UNIT_FLEET][from_loc][exact]) {
            return exact;  // Exact match is reachable by fleet
        }
        // Exact match exists but not reachable by fleet - fall through to coast inference
    }

    // Check if it's a split coast location - try to infer the correct coast
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
                // Check if this location is reachable for fleets
                if (map->adjacency_cache[UNIT_FLEET][from_loc][i]) {
                    matching_coasts[num_matching++] = i;
                }
            }
        }

        // If exactly one reachable coast, return it (DATC 6.B.2)
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
    game->powers[5].units[1] = (Unit){UNIT_FLEET, 81, 5, 0}; // F STP/SC
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

    // Parent locations (BUL, SPA, STP) now have full adjacencies,
    // so no need for lowercase fallback - direct lookup works
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

        // For fleets, use default_coast for coast inference (DATC 6.B.2)
        // default_coast uses fleet adjacency cache to find valid destinations
        if (order->unit_type == UNIT_FLEET) {
            int inferred_loc = default_coast(game->map, order->unit_location, token);
            if (inferred_loc >= 0) {
                dest_loc = inferred_loc;
            }
        }

        // If no valid destination, the move is invalid
        if (dest_loc < 0) return -1;
        order->target_location = dest_loc;

        // Check for VIA keyword (forces convoy for adjacent moves)
        // DATC 4.A.3: "VIA" explicitly requests convoy route
        token = strtok(NULL, " ");
        if (token && (strcmp(token, "VIA") == 0 || strcmp(token, "VIA CONVOY") == 0)) {
            order->explicit_convoy = 1;
        }
        return 0;

    } else if (strcmp(token, "S") == 0 || strcmp(token, "SUPPORT") == 0 ||
               strcmp(token, "SUPPORTS") == 0) {
        // SUPPORT order
        // Format: "A PAR S A MAR - BUR" or "A PAR S MAR - BUR" (unit type optional)

        // Parse next token - could be unit type (A/F) or location
        token = strtok(NULL, " ");
        if (!token) return -1;

        // Check if this token is a unit type or a location
        int supported_loc;
        if ((token[0] == 'A' || token[0] == 'F') && strlen(token) == 1) {
            // Token is a unit type - skip it and read location
            token = strtok(NULL, " ");
            if (!token) return -1;
            supported_loc = find_location_by_name(game->map, token);
        } else {
            // Token is the location directly (no unit type specified)
            supported_loc = find_location_by_name(game->map, token);
        }
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

            // DATC 6.B.7-10: Support orders don't require coast specification
            // If destination not found by exact name, try to find any coast variant
            if (dest_loc < 0 && strlen(token) == 3) {
                // Try to find any location that matches this base name (e.g., SPA/NC for SPA)
                for (int i = 0; i < game->map->num_locations; i++) {
                    if (strncmp(game->map->locations[i].name, token, 3) == 0) {
                        dest_loc = i;
                        break;
                    }
                }
            }

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

        // Parse convoyed unit type (should be A) or location directly
        // DATC 6.A.7: Allow "C LON - BEL" without explicit "A" prefix
        token = strtok(NULL, " ");
        if (!token) return -1;

        // Check if this token is a unit type or a location
        int convoyed_loc;
        if ((token[0] == 'A' || token[0] == 'F') && strlen(token) == 1) {
            // Token is a unit type - read next token for location
            token = strtok(NULL, " ");
            if (!token) return -1;
            convoyed_loc = find_location_by_name(game->map, token);
        } else {
            // Token is the location directly (no unit type specified)
            convoyed_loc = find_location_by_name(game->map, token);
        }
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
            // DATC 6.D.34: Can't support a move to where you're already standing
            // A unit can only support to areas it can move to itself, and you can't move to yourself
            if (order->type == ORDER_SUPPORT_MOVE) {
                int dest_parent = get_parent_location(map, order->dest_location);
                int unit_parent = get_parent_location(map, unit->location);
                if (dest_parent == unit_parent) {
                    return -1;  // Can't support move to own location
                }
                // DATC 6.A.5: Can't support a move where the supported unit moves to its own location
                int supported_parent = get_parent_location(map, order->target_unit_location);
                if (dest_parent == supported_parent) {
                    return -1;  // Can't support move to same location (illegal move)
                }
            }
            // Check supporting unit can reach the target location or destination
            // (supporting unit must be adjacent to either the supported unit or the destination)
            int can_support = 0;
            if (can_move(map, unit->type, unit->location, order->target_unit_location)) {
                can_support = 1;  // Adjacent to supported unit
            }
            if (order->dest_location >= 0 && order->dest_location < map->num_locations) {
                // Check if supporter can reach destination or any of its coasts (DATC 6.B.7-9)
                if (can_move(map, unit->type, unit->location, order->dest_location)) {
                    can_support = 1;  // Adjacent to destination
                } else {
                    // Try all coasts of the destination
                    int coasts[10];
                    int num_coasts;
                    find_coasts(map, order->dest_location, coasts, &num_coasts);
                    for (int c = 0; c < num_coasts; c++) {
                        if (can_move(map, unit->type, unit->location, coasts[c])) {
                            can_support = 1;  // Adjacent to a coast of destination
                            break;
                        }
                    }
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
            // DATC 6.A.7: Only ARMIES can be convoyed
            // Check if there's an army at the target_unit_location (any power)
            {
                int found_army = 0;
                for (int check_p = 0; check_p < MAX_POWERS; check_p++) {
                    Power* check_power = &game->powers[check_p];
                    for (int check_u = 0; check_u < check_power->num_units; check_u++) {
                        int unit_loc_parent = get_parent_location(map, check_power->units[check_u].location);
                        int target_parent = get_parent_location(map, order->target_unit_location);
                        if (unit_loc_parent == target_parent &&
                            check_power->units[check_u].type == UNIT_ARMY) {
                            found_army = 1;
                            break;
                        }
                    }
                    if (found_army) break;
                }
                if (!found_army) {
                    return -1;  // No army at location to convoy (DATC 6.A.7)
                }
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

    // Get all coast variants of start and end (for PORT locations like SPA, BUL, STP)
    int start_coasts[10], num_start_coasts;
    int end_coasts[10], num_end_coasts;
    find_coasts(map, start, start_coasts, &num_start_coasts);
    find_coasts(map, end, end_coasts, &num_end_coasts);

    // BFS to find path through convoying fleets
    // Queue for BFS
    int queue[MAX_LOCATIONS];
    int visited[MAX_LOCATIONS] = {0};
    int queue_start = 0, queue_end = 0;

    // Start with convoying fleets adjacent to start location (or any of its coasts)
    for (int i = 0; i < num_convoying_fleets; i++) {
        int fleet_loc = convoying_fleets[i];

        // Check if this fleet is adjacent to start or any of its coasts
        int adjacent = 0;
        for (int sc = 0; sc < num_start_coasts && !adjacent; sc++) {
            Location* coast_loc = &map->locations[start_coasts[sc]];
            for (int j = 0; j < coast_loc->num_adjacent; j++) {
                if (coast_loc->adjacencies[j] == fleet_loc) {
                    adjacent = 1;
                    break;
                }
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

        // Check if this fleet is adjacent to destination or any of its coasts
        for (int i = 0; i < current_loc->num_adjacent; i++) {
            int adj = current_loc->adjacencies[i];
            for (int ec = 0; ec < num_end_coasts; ec++) {
                if (adj == end_coasts[ec]) {
                    return 1;  // Found a path!
                }
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
    int convoy_paradox;  // 1 if disrupted due to Szykman paradox rule
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


static void collect_movement_orders(GameState* game, MoveAttempt* attempts, int* num_attempts,
                                     SupportOrder* supports, int* num_supports) {
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

            // If no exact match and this is a fleet, try to find unit at other coasts (DATC 6.B.10)
            if (unit_idx < 0 && order->unit_type == UNIT_FLEET) {
                int parent_loc = get_parent_location(game->map, order->unit_location);
                int coasts[10];
                int num_coasts;
                find_coasts(game->map, parent_loc, coasts, &num_coasts);

                for (int c = 0; c < num_coasts; c++) {
                    for (int u = 0; u < power->num_units; u++) {
                        if (power->units[u].location == coasts[c] && power->units[u].type == UNIT_FLEET) {
                            unit_idx = u;
                            // Update order location to match actual unit location
                            order->unit_location = power->units[u].location;
                            break;
                        }
                    }
                    if (unit_idx >= 0) break;
                }
            }
            
            if (unit_idx < 0) {
                continue;  // Order for non-existent unit
            }
            
            // Handle different order types
            if (order->type == ORDER_HOLD || order->type == ORDER_MOVE) {
                MoveAttempt* attempt = &attempts[(*num_attempts)++];
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

                    // Check if this is a convoyed move (army with valid convoy path)
                    // DATC 4.A.3: Adjacent moves - "kidnapping" rule
                    // - If NOT adjacent: must use convoy
                    // - If adjacent: only use convoy if:
                    //   a) SAME power offers it (voluntary convoy), OR
                    //   b) "VIA" keyword was used (explicit convoy request)
                    if (attempt->to_location != -1 && order->unit_type == UNIT_ARMY) {
                        int can_move_directly = can_move(game->map, UNIT_ARMY,
                                                         order->unit_location,
                                                         order->target_location);

                        if (!can_move_directly) {
                            // Non-adjacent: must use convoy if path exists
                            if (is_convoyed_move(game, order->unit_location, order->target_location)) {
                                attempt->is_convoyed = 1;
                            }
                        } else {
                            // Adjacent: check for convoy usage
                            int use_convoy = 0;

                            // Check if "VIA" keyword was used (explicit convoy request)
                            if (order->explicit_convoy) {
                                use_convoy = 1;
                            } else {
                                // Check if SAME power offers convoy (voluntary convoy)
                                for (int cp = 0; cp < MAX_POWERS; cp++) {
                                    if (cp != p) continue;  // Only check same power
                                    Power* convoy_power = &game->powers[cp];
                                    for (int co = 0; co < convoy_power->num_orders; co++) {
                                        Order* convoy_order = &convoy_power->orders[co];
                                        if (convoy_order->type == ORDER_CONVOY &&
                                            convoy_order->target_unit_location == order->unit_location &&
                                            convoy_order->dest_location == order->target_location) {
                                            use_convoy = 1;
                                            break;
                                        }
                                    }
                                    if (use_convoy) break;
                                }
                            }

                            if (use_convoy &&
                                is_convoyed_move(game, order->unit_location, order->target_location)) {
                                attempt->is_convoyed = 1;
                            }
                        }
                    }
                }
            }
            else if (order->type == ORDER_SUPPORT_HOLD || order->type == ORDER_SUPPORT_MOVE) {
                // Collect support orders
                SupportOrder* support = &supports[(*num_supports)++];
                support->supporter_power = p;
                support->supporter_location = order->unit_location;
                support->supported_location = order->target_unit_location;
                support->destination = (order->type == ORDER_SUPPORT_MOVE) ?
                                      order->dest_location : order->target_unit_location;
                support->is_valid = (validate_order(game, p, order) == 0);
                support->is_cut = 0;  // Determined later
                support->order_idx = o;  // Track which order this came from

                // Also add this unit to move attempts as HOLD (supporting units hold their position)
                MoveAttempt* attempt = &attempts[(*num_attempts)++];
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
                MoveAttempt* attempt = &attempts[(*num_attempts)++];
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

}

static void detect_support_cuts(GameState* game, MoveAttempt* attempts, int num_attempts,
                                 SupportOrder* supports, int num_supports) {
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
                    // Exception: defender cannot cut support for attack on itself (DATC 6.D.15)
                    // If attacker's origin is the destination of the supported move, don't cut
                    int support_dest_parent = get_parent_location(game->map, support->destination);

                    if (attacker_src_parent != support_dest_parent) {
                        // Exception: own units don't cut support
                        if (attacker->unit_power != support->supporter_power) {
                            support->is_cut = 1;
                        }
                    }
                }
            }
        }
    }
    
}

static void calculate_strengths(GameState* game, MoveAttempt* attempts, int num_attempts,
                                 SupportOrder* supports, int num_supports) {
    // Step 3a: Invalidate hold supports on moving units
    // A unit that has a move order cannot receive hold support (DATC 6.D.7)
    for (int s = 0; s < num_supports; s++) {
        SupportOrder* support = &supports[s];

        if (!support->is_valid) {
            continue;
        }

        // Check if this is a hold support (destination == supported_location)
        if (support->destination == support->supported_location) {
            // This is a hold support - check if supported unit has a move order
            for (int i = 0; i < num_attempts; i++) {
                MoveAttempt* attempt = &attempts[i];

                if (attempt->from_location == support->supported_location) {
                    // Found the unit - check if it has a valid move order
                    if (attempt->order_idx >= 0) {
                        Power* power = &game->powers[attempt->unit_power];
                        Order* order = &power->orders[attempt->order_idx];
                        // DATC 6.D.28-32: Impossible moves (to_location < 0) are treated as holds
                        // Hold support is only invalid if unit is actually moving (to_location >= 0)
                        if (order->type == ORDER_MOVE && attempt->to_location >= 0 && attempt->is_valid) {
                            // Unit has a VALID move order and is actually moving - hold support is invalid
                            support->is_valid = 0;
                            break;
                        }
                    }
                }
            }
        }
    }

    // Step 3b: Calculate attack and defense strengths with valid supports
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
                // Compare parent locations for coasts (DATC 6.B.7-9)
                int support_from_parent = get_parent_location(game->map, support->supported_location);
                int attempt_from_parent = get_parent_location(game->map, attempt->from_location);
                int support_to_parent = get_parent_location(game->map, support->destination);
                int attempt_to_parent = get_parent_location(game->map, attempt->to_location);

                if (support_from_parent == attempt_from_parent &&
                    support_to_parent == attempt_to_parent) {
                    
                    // DATC support on own unit rules:
                    // 6.D.12-14: Can't support dislodging own HOLDING unit
                    // 6.E.8: Can't support attack when own unit is in head-to-head with attacker
                    // 6.E.9: If own unit vacates to EMPTY location, support is valid
                    // 6.E.12: If there are MULTIPLE attackers (potential standoff), support is valid
                    // 6.D.13: If own unit bounces AND only one attacker, support is void
                    //
                    // Heuristic: Support is void only if:
                    // - Own unit is holding, OR
                    // - Own unit is in head-to-head with the supported attacker, OR
                    // - Own unit can't vacate AND there's only ONE attacker
                    int own_unit_at_dest = 0;
                    for (int j = 0; j < num_attempts; j++) {
                        int j_from_parent = get_parent_location(game->map, attempts[j].from_location);
                        int dest_parent = get_parent_location(game->map, attempt->to_location);
                        if (j_from_parent == dest_parent &&
                            attempts[j].unit_power == support->supporter_power) {
                            // Own unit at destination
                            int own_to_loc = attempts[j].to_location;
                            int attacker_from_parent = get_parent_location(game->map, attempt->from_location);

                            if (own_to_loc < 0) {
                                // Unit is holding - support is void
                                own_unit_at_dest = 1;
                            } else {
                                int own_to_parent = get_parent_location(game->map, own_to_loc);
                                if (own_to_parent == j_from_parent) {
                                    // Move to same location (VOID order) - support is void
                                    own_unit_at_dest = 1;
                                } else if (own_to_parent == attacker_from_parent &&
                                           !attempts[j].is_convoyed && !attempt->is_convoyed) {
                                    // Head-to-head with the supported attacker - support is void
                                    // EXCEPT: convoy swap (6.E.11, 6.G) - if either unit is
                                    // convoyed, they can pass each other
                                    own_unit_at_dest = 1;
                                } else {
                                    // Unit is moving elsewhere - check various conditions
                                    // 6.E.9: If own unit vacates to empty location, support is valid
                                    // 6.E.12: If multiple attackers (beleaguered garrison), support is valid
                                    // 6.D.13: If own unit's move will fail AND only one attacker, support is void
                                    // 6.C.2: Circular movements - if unit is part of cycle, it will vacate

                                    int dest_blocked_by_stationary = 0;
                                    int other_attackers = 0;

                                    for (int k = 0; k < num_attempts; k++) {
                                        if (k == j) continue;
                                        int k_from_parent = get_parent_location(game->map, attempts[k].from_location);
                                        int k_to_parent = attempts[k].to_location >= 0 ?
                                                          get_parent_location(game->map, attempts[k].to_location) : -1;

                                        // Check if unit at own's destination is holding (stationary blocker)
                                        if (k_from_parent == own_to_parent) {
                                            if (attempts[k].to_location < 0) {
                                                // Unit is holding - blocks vacate
                                                dest_blocked_by_stationary = 1;
                                            }
                                            // If unit is moving (to_location >= 0), it might vacate
                                            // (circular movement or other move) - don't block
                                        }

                                        // Check if another unit is moving TO own's destination (head-to-head)
                                        // Only block if attacker is from different location and would contest
                                        if (k_to_parent == own_to_parent && k_from_parent != own_to_parent) {
                                            // Another unit moving to same destination - could cause bounce
                                            // But in circular movements, this is handled by cycle resolution
                                            // Only consider it blocked if that unit is NOT the own unit itself
                                            if (k_from_parent != j_from_parent) {
                                                dest_blocked_by_stationary = 1;
                                            }
                                        }

                                        // Count other attackers on own unit's location
                                        if (k_to_parent == j_from_parent && k_from_parent != j_from_parent && k != i) {
                                            other_attackers++;
                                        }
                                    }

                                    if (!dest_blocked_by_stationary) {
                                        // Destination can be vacated - support is valid
                                        // (6.E.9 case, 6.C.2 circular movement)
                                    } else if (other_attackers > 0) {
                                        // Multiple attackers - potential standoff, support is valid
                                        // (6.E.12 case - support creates beleaguered garrison)
                                    } else {
                                        // Destination blocked AND only one attacker - support would
                                        // be the deciding factor in dislodging, void it
                                        // (6.D.13 case)
                                        own_unit_at_dest = 1;
                                    }
                                }
                            }
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
        int dest_parent = get_parent_location(game->map, destination);

        // First, check for head-to-head battle (must check ALL units, not just those attacking same dest)
        int head_to_head_opponent_idx = -1;
        for (int j = 0; j < num_attempts; j++) {
            if (j == i) continue;
            MoveAttempt* other = &attempts[j];

            if (!other->is_valid || other->to_location < 0) {
                continue;
            }

            // Check for head-to-head: other unit going from our dest to our source
            // For split coasts, check parent locations (e.g., BUL/SC <-> BUL/EC)
            int other_from_parent = get_parent_location(game->map, other->from_location);
            int other_to_parent = get_parent_location(game->map, other->to_location);
            int dest_parent = get_parent_location(game->map, destination);
            int src_parent = get_parent_location(game->map, source);

            if (other_from_parent == dest_parent && other_to_parent == src_parent) {
                head_to_head_opponent_idx = j;
                break;  // Found head-to-head opponent
            }
        }

        // Find all competing moves to this destination
        int max_attack_strength = 0;
        int num_with_max_strength = 0;

        for (int j = 0; j < num_attempts; j++) {
            MoveAttempt* other = &attempts[j];

            if (!other->is_valid || other->to_location < 0) {
                continue;
            }

            // Compare parent locations for split coast handling (DATC 6.B.4-7)
            int other_dest_parent = get_parent_location(game->map, other->to_location);
            if (other_dest_parent != dest_parent) {
                continue;  // Not attacking this destination
            }

            // DATC 6.F.8: Don't count convoyed moves whose convoy will be disrupted
            if (other->is_convoyed) {
                // Check if convoy path will be disrupted
                int convoy_will_fail = 0;
                int valid_fleets[MAX_LOCATIONS];
                int num_valid_fleets = 0;

                // Collect all convoying fleets that WON'T be dislodged
                for (int p = 0; p < MAX_POWERS; p++) {
                    Power* pwr = &game->powers[p];
                    for (int o = 0; o < pwr->num_orders; o++) {
                        Order* ord = &pwr->orders[o];
                        if (ord->type == ORDER_CONVOY &&
                            ord->target_unit_location == other->from_location &&
                            ord->dest_location == other->to_location) {
                            int fleet_loc = ord->unit_location;

                            // Check if this fleet is being dislodged
                            int fleet_dislodged = 0;
                            for (int k = 0; k < num_attempts; k++) {
                                if (attempts[k].to_location == fleet_loc && k != j) {
                                    // Fleet is being attacked
                                    // Check if attack will succeed (attacker strength > fleet defense)
                                    int fleet_def = 1;  // Base defense
                                    for (int s = 0; s < num_supports; s++) {
                                        if (!supports[s].is_cut && supports[s].is_valid &&
                                            supports[s].supported_location == fleet_loc &&
                                            supports[s].destination == fleet_loc) {
                                            fleet_def++;
                                        }
                                    }
                                    if (attempts[k].attack_strength > fleet_def) {
                                        fleet_dislodged = 1;
                                        break;
                                    }
                                }
                            }

                            if (!fleet_dislodged && can_fleet_convoy(game->map, fleet_loc)) {
                                valid_fleets[num_valid_fleets++] = fleet_loc;
                            }
                        }
                    }
                }

                // Check if a valid convoy path still exists
                if (num_valid_fleets == 0 ||
                    !find_convoy_path(game->map, other->from_location,
                                     other->to_location, valid_fleets, num_valid_fleets)) {
                    convoy_will_fail = 1;
                }

                if (convoy_will_fail) {
                    continue;  // Disrupted convoy doesn't compete for destination
                }
            }

            // Track max attack strength
            if (other->attack_strength > max_attack_strength) {
                max_attack_strength = other->attack_strength;
                num_with_max_strength = 1;
            } else if (other->attack_strength == max_attack_strength) {
                num_with_max_strength++;
            }
        }
        
        // Find defender at destination (if any)
        // Compare parent locations for split coast handling (DATC 6.B)
        int defender_idx = -1;
        int defender_strength = 0;
        for (int j = 0; j < num_attempts; j++) {
            int defender_loc_parent = get_parent_location(game->map, attempts[j].from_location);
            if (defender_loc_parent == dest_parent) {
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
                    // Head-to-head battle detected
                    MoveAttempt* opponent = &attempts[head_to_head_opponent_idx];

                    // DATC 6.G: Convoy swap logic
                    if (attacker->is_convoyed || opponent->is_convoyed) {
                        // At least one is convoyed - check if it's a swap or a dislodge
                        // Swap: both units pass through each other (neither dislodges)
                        // Dislodge: stronger unit wins and dislodges weaker (even if convoyed)

                        // DATC 6.G.10: If convoyed attacker has strength > opponent's defense,
                        // the convoyed unit wins and dislodges the opponent (NOT a swap)
                        if (attacker->attack_strength > opponent->defend_strength &&
                            attacker->unit_power != opponent->unit_power) {
                            // Convoyed attacker wins - dislodges opponent
                            // BUT: if attacker is convoyed, defer to Step 6c for paradox check
                            if (!attacker->is_convoyed) {
                                attacker->can_move = 1;
                            }
                            // Opponent will be dislodged (handled in Step 6)
                        } else {
                            // Possible swap - let cycle detection handle it
                            // The cycle detection will check for external blockers
                            continue;
                        }
                    } else {
                        // Direct head-to-head battle (no convoy)
                        // DATC 6.E.2: Prevent self-dislodgement in head-to-head battles
                        if (attacker->attack_strength > opponent->attack_strength &&
                            attacker->unit_power != opponent->unit_power) {
                            // We win - can move, they're dislodged (not same power)
                            attacker->can_move = 1;
                            // Mark for dislodgement (tracked later)
                        } else {
                            // Equal or weaker, or same power - both bounce
                            continue;
                        }
                    }
                } else {
                    // Not head-to-head, defender holding or moving elsewhere
                    if (attacker->attack_strength > defender_strength) {
                        // Successful attack - defender dislodged
                        // EXCEPT: convoyed moves need paradox check first (Step 6b/6c)
                        if (!attacker->is_convoyed) {
                            attacker->can_move = 1;
                        }
                        // Mark defender for dislodgement (done in Step 6)
                    } else {
                        // Attack bounced
                        continue;
                    }
                }
            } else {
                // No defender - move succeeds
                // EXCEPT: convoyed moves need paradox check first (Step 6b/6c)
                if (!attacker->is_convoyed) {
                    attacker->can_move = 1;
                }
            }
        }
    }
    
}

static void resolve_conflicts_and_circular(GameState* game, MoveAttempt* attempts, int num_attempts,
                                             SupportOrder* supports, int num_supports) {
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
                int cycle_dest_parent = get_parent_location(game->map, cycle_dest);

                // Check for external attackers
                for (int j = 0; j < num_attempts; j++) {
                    if (!attempts[j].is_valid || attempts[j].to_location < 0) {
                        continue;
                    }

                    // Compare parent locations for split coast handling (DATC 6.B)
                    int j_dest_parent = get_parent_location(game->map, attempts[j].to_location);
                    if (j_dest_parent == cycle_dest_parent) {
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

            // DATC 6.E.2: Check for same-power swap (2-unit cycle)
            // Same-power units cannot swap (self-dislodgement prevention)
            if (cycle_len == 2 && can_resolve_cycle) {
                int unit1_power = attempts[cycle_indices[0]].unit_power;
                int unit2_power = attempts[cycle_indices[1]].unit_power;
                if (unit1_power == unit2_power) {
                    // Same power trying to swap - not allowed
                    can_resolve_cycle = 0;
                }

                // Head-to-head check: in a 2-unit cycle without convoy, if neither has enough
                // strength to dislodge the other, it's a contested head-to-head and both bounce
                // DATC 6.E.3: Two units can only swap via convoy, not by direct move
                if (can_resolve_cycle) {
                    MoveAttempt* unit1 = &attempts[cycle_indices[0]];
                    MoveAttempt* unit2 = &attempts[cycle_indices[1]];
                    // If neither unit is convoying, it's a direct swap attempt (not allowed)
                    if (!unit1->is_convoyed && !unit2->is_convoyed) {
                        // Direct swap - both bounce unless one wins the head-to-head
                        if (unit1->attack_strength <= unit2->defend_strength &&
                            unit2->attack_strength <= unit1->defend_strength) {
                            // Neither can dislodge the other - contested head-to-head
                            can_resolve_cycle = 0;
                        }
                    }
                }
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
            // Compare parent locations for split coasts (DATC 6.B.4-7)
            int is_strongest = 1;
            int max_str = attempt->attack_strength;
            int num_at_max = 1;
            int dest_parent = get_parent_location(game->map, destination);

            for (int j = 0; j < num_attempts; j++) {
                if (i == j) continue;
                if (!attempts[j].is_valid || attempts[j].to_location < 0) {
                    continue;
                }

                // Compare parent locations - moves to different coasts conflict (DATC 6.B.4)
                int j_dest_parent = get_parent_location(game->map, attempts[j].to_location);
                if (j_dest_parent != dest_parent) {
                    continue;
                }

                // DATC 6.E.1: Dislodged unit has no effect on attacker's area
                // Check if this competing attacker was dislodged (lost a battle at its origin)
                int attacker_dislodged = 0;
                // First check the dislodged list (for units dislodged in step 6)
                for (int d = 0; d < game->num_dislodged; d++) {
                    if (game->dislodged[d].power_id == attempts[j].unit_power &&
                        game->dislodged[d].from_location == attempts[j].from_location) {
                        attacker_dislodged = 1;
                        break;
                    }
                }
                // Also check if a successful mover is taking this attacker's origin
                // (means they lost a head-to-head or were attacked and lost)
                if (!attacker_dislodged) {
                    for (int k = 0; k < num_attempts; k++) {
                        if (attempts[k].can_move &&
                            attempts[k].to_location == attempts[j].from_location &&
                            attempts[k].attack_strength > attempts[j].defend_strength) {
                            // DATC 6.G.10 / 4.A.7 choice (b): If the attacker arrived via convoy,
                            // the dislodged unit CAN still contest destinations (including
                            // the attacker's origin) because there's no head-to-head battle.
                            // Only direct (non-convoyed) dislodgement prevents contesting.
                            if (!attempts[k].is_convoyed) {
                                // Direct dislodgement - unit can't contest
                                attacker_dislodged = 1;
                            }
                            // If attacker used convoy, dislodged unit CAN still contest
                            break;
                        }
                    }
                }
                if (attacker_dislodged) {
                    continue;  // Dislodged attacker doesn't contest destination
                }

                // DATC 6.F.8: Convoyed moves whose convoy will be disrupted don't contest
                // Check if this is a convoyed move and its convoying fleet(s) will be dislodged
                if (attempts[j].is_convoyed) {
                    // Check if convoy path will be disrupted
                    int convoy_will_fail = 0;
                    int valid_fleets[MAX_LOCATIONS];
                    int num_valid_fleets = 0;

                    // Collect all convoying fleets that WON'T be dislodged
                    for (int p = 0; p < MAX_POWERS; p++) {
                        Power* pwr = &game->powers[p];
                        for (int o = 0; o < pwr->num_orders; o++) {
                            Order* ord = &pwr->orders[o];
                            if (ord->type == ORDER_CONVOY &&
                                ord->target_unit_location == attempts[j].from_location &&
                                ord->dest_location == attempts[j].to_location) {
                                int fleet_loc = ord->unit_location;

                                // Check if this fleet is being dislodged
                                int fleet_dislodged = 0;
                                for (int k = 0; k < num_attempts; k++) {
                                    if (attempts[k].can_move &&
                                        attempts[k].to_location == fleet_loc) {
                                        // Fleet is being attacked by successful mover
                                        // Check if attack strength > fleet defense
                                        int fleet_def = 1;  // Base defense
                                        for (int s = 0; s < num_supports; s++) {
                                            if (!supports[s].is_cut && supports[s].is_valid &&
                                                supports[s].supported_location == fleet_loc &&
                                                supports[s].destination == fleet_loc) {
                                                fleet_def++;
                                            }
                                        }
                                        if (attempts[k].attack_strength > fleet_def) {
                                            fleet_dislodged = 1;
                                            break;
                                        }
                                    }
                                }

                                if (!fleet_dislodged && can_fleet_convoy(game->map, fleet_loc)) {
                                    valid_fleets[num_valid_fleets++] = fleet_loc;
                                }
                            }
                        }
                    }

                    // Check if a valid convoy path still exists
                    if (num_valid_fleets == 0 ||
                        !find_convoy_path(game->map, attempts[j].from_location,
                                         attempts[j].to_location, valid_fleets, num_valid_fleets)) {
                        convoy_will_fail = 1;
                    }

                    if (convoy_will_fail) {
                        continue;  // Disrupted convoy doesn't contest destination
                    }
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
                // Skip if defender is successfully moving away (DATC 6.E.1, 6.E.9)
                if (defender->can_move && defender->to_location != -1 &&
                    defender->to_location != attacker->from_location) {
                    // Defender is vacating, not in head-to-head, no dislodgement
                    break;
                }

                // DATC 6.G: Convoy swap - if defender is in head-to-head AND either unit
                // is convoyed, they swap positions (no dislodgement)
                if (defender->can_move && defender->to_location == attacker->from_location) {
                    // This is a head-to-head situation
                    if (attacker->is_convoyed || defender->is_convoyed) {
                        // Convoy swap - no dislodgement
                        break;
                    }
                }

                // It's dislodged if we have strength to dislodge it
                if (attacker->attack_strength > defender->defend_strength) {
                    // Check if already dislodged (avoid duplicates)
                    int already_dislodged = 0;
                    for (int d = 0; d < game->num_dislodged; d++) {
                        if (game->dislodged[d].power_id == defender->unit_power &&
                            game->dislodged[d].from_location == defender->from_location) {
                            already_dislodged = 1;
                            break;
                        }
                    }

                    if (!already_dislodged && game->num_dislodged < MAX_UNITS) {
                        // Record dislodgement
                        DislodgedUnit* dislodged = &game->dislodged[game->num_dislodged++];
                        dislodged->type = defender->unit_type;
                        dislodged->power_id = defender->unit_power;
                        dislodged->from_location = defender->from_location;
                        dislodged->dislodged_by_location = attacker->from_location;
                        dislodged->attacker_used_convoy = attacker->is_convoyed ? 1 : 0;
                        dislodged->num_possible_retreats = 0;  // Calculate later in retreat phase
                    }

                    // Mark defender as not able to stay
                    // We'll remove the unit when applying moves
                }
                break;
            }
        }
    }

    // Step 6a: Check for units that failed to move but are being attacked at their origin
    // This handles: bounced moves, void orders, and any unit that stays put but is attacked
    for (int i = 0; i < num_attempts; i++) {
        MoveAttempt* stationary_unit = &attempts[i];

        // Skip units that successfully moved
        if (stationary_unit->can_move) {
            continue;
        }

        // This unit is staying at its location - check if it's being attacked
        int strongest_attacker_idx = -1;
        int strongest_attack_strength = stationary_unit->defend_strength;

        for (int j = 0; j < num_attempts; j++) {
            MoveAttempt* attacker = &attempts[j];

            if (!attacker->can_move || attacker->to_location < 0) {
                continue;  // Not a successful attack
            }

            // Check if this attacker is moving to the stationary unit's location
            if (attacker->to_location == stationary_unit->from_location) {
                // Found an attacker - check if it's stronger
                if (attacker->attack_strength > strongest_attack_strength) {
                    strongest_attack_strength = attacker->attack_strength;
                    strongest_attacker_idx = j;
                }
            }
        }

        // If we found a strong enough attacker, dislodge this unit
        if (strongest_attacker_idx >= 0) {
            MoveAttempt* attacker = &attempts[strongest_attacker_idx];

            // Check if already dislodged (avoid duplicates)
            int already_dislodged = 0;
            for (int d = 0; d < game->num_dislodged; d++) {
                if (game->dislodged[d].power_id == stationary_unit->unit_power &&
                    game->dislodged[d].from_location == stationary_unit->from_location) {
                    already_dislodged = 1;
                    break;
                }
            }

            if (!already_dislodged && game->num_dislodged < MAX_UNITS) {
                // Record dislodgement
                DislodgedUnit* dislodged = &game->dislodged[game->num_dislodged++];
                dislodged->type = stationary_unit->unit_type;
                dislodged->power_id = stationary_unit->unit_power;
                dislodged->from_location = stationary_unit->from_location;
                dislodged->dislodged_by_location = attacker->from_location;
                dislodged->attacker_used_convoy = attacker->is_convoyed ? 1 : 0;
                dislodged->num_possible_retreats = 0;  // Calculate later in retreat phase
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

        // If convoy is valid (or paradox), check if it would cut any supports
        // DATC 6.F.16-18 (Pandin's Paradox): If cutting support would tip balance and dislodge
        // the convoying fleet, it's a paradox - convoy fails, support NOT cut.
        if (!attempt->convoy_disrupted) {
            // Check for convoy paradoxes (Szykman rule):
            // 1. Convoy destination supports convoy fleet's DEFENSE - cutting would allow dislodge
            // 2. Convoy destination supports ATTACK on convoy fleet - beleaguered garrison
            int is_convoy_paradox = 0;

            int convoy_dest_parent = get_parent_location(game->map, attempt->to_location);

            for (int f = 0; f < num_fleets && !is_convoy_paradox; f++) {
                int fleet_loc = convoying_fleets[f];

                // Find the convoying fleet's MoveAttempt to get its defense strength
                MoveAttempt* fleet_attempt = NULL;
                for (int j = 0; j < num_attempts; j++) {
                    if (attempts[j].from_location == fleet_loc) {
                        fleet_attempt = &attempts[j];
                        break;
                    }
                }
                if (!fleet_attempt) continue;

                // Find max attack strength on this fleet
                int max_attack_str = 0;
                int num_max_attackers = 0;
                for (int j = 0; j < num_attempts; j++) {
                    if (attempts[j].to_location == fleet_loc && attempts[j].is_valid) {
                        if (attempts[j].attack_strength > max_attack_str) {
                            max_attack_str = attempts[j].attack_strength;
                            num_max_attackers = 1;
                        } else if (attempts[j].attack_strength == max_attack_str) {
                            num_max_attackers++;
                        }
                    }
                }

                if (max_attack_str == 0) continue;  // No attacks on this fleet

                // Case 1: Check if convoy destination is supporting the convoy fleet's DEFENSE
                // (6.F.18 - betrayal paradox)
                for (int s = 0; s < num_supports && !is_convoy_paradox; s++) {
                    if (!supports[s].is_valid || supports[s].is_cut) continue;

                    int supporter_parent = get_parent_location(game->map, supports[s].supporter_location);
                    int supported_loc = supports[s].supported_location;

                    // Is this support FROM the convoy destination FOR the convoy fleet?
                    if (supporter_parent == convoy_dest_parent && supported_loc == fleet_loc) {
                        // This support is helping the convoy fleet's defense
                        // If cut, fleet's defense drops by 1
                        int reduced_defense = fleet_attempt->defend_strength - 1;

                        // Would the attacker now dislodge the fleet?
                        if (max_attack_str > reduced_defense) {
                            // Cutting this support would allow dislodgement - PARADOX!
                            is_convoy_paradox = 1;
                        }
                    }
                }

                // Case 2: Check if convoy destination supports ATTACK on convoy fleet
                // (Beleaguered garrison paradox - 6.F.16-17)
                for (int s = 0; s < num_supports && !is_convoy_paradox; s++) {
                    if (!supports[s].is_valid || supports[s].is_cut) continue;

                    int supporter_parent = get_parent_location(game->map, supports[s].supporter_location);
                    int supported_dest = supports[s].destination;

                    // Is this support FROM the convoy destination FOR an attack on the fleet?
                    if (supporter_parent == convoy_dest_parent && supported_dest == fleet_loc) {
                        // This support is helping attack the convoy fleet
                        // If cut, attack strength drops - check if this changes outcome

                        // If beleaguered (2+ equal attackers) and cutting changes balance
                        if (num_max_attackers >= 2 &&
                            fleet_attempt->defend_strength <= max_attack_str) {
                            // Cutting would break the beleaguered tie - PARADOX!
                            is_convoy_paradox = 1;
                        }
                    }
                }
            }

            if (is_convoy_paradox) {
                // Szykman rule: convoy fails, support NOT cut
                attempt->convoy_disrupted = 1;
                attempt->convoy_paradox = 1;  // Mark as paradox disruption
                attempt->can_move = 0;
                continue;  // Skip cutting support
            }

            // Normal case: check if it cuts any supports
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
        int dest_parent = get_parent_location(game->map, destination);

        // Find defender at destination
        int defender_idx = -1;
        int defender_strength = 0;
        for (int j = 0; j < num_attempts; j++) {
            int defender_loc_parent = get_parent_location(game->map, attempts[j].from_location);
            if (defender_loc_parent == dest_parent) {
                defender_idx = j;
                defender_strength = attempts[j].defend_strength;
                break;
            }
        }

        // Check if destination is vacant, being vacated, or can be dislodged
        int can_enter = 0;
        if (defender_idx < 0) {
            // No defender - can move
            can_enter = 1;
        } else {
            MoveAttempt* defender = &attempts[defender_idx];
            if (defender->can_move && defender->to_location >= 0) {
                // Defender is moving away - can move
                can_enter = 1;
            } else {
                // Defender is holding - can we dislodge?
                if (attempt->attack_strength > defender_strength &&
                    attempt->unit_power != defender->unit_power) {
                    // Can dislodge (and not same power)
                    can_enter = 1;
                }
            }
        }

        if (!can_enter) {
            continue;  // Can't enter destination
        }

        // Check if we're the strongest attacker (among ALL moves to this destination)
        int is_strongest = 1;
        int max_str = attempt->attack_strength;
        int num_at_max = 1;

        for (int j = 0; j < num_attempts; j++) {
            if (i == j) continue;
            if (!attempts[j].is_valid || attempts[j].to_location < 0) {
                continue;
            }

            // Compare parent locations for split coasts
            int j_dest_parent = get_parent_location(game->map, attempts[j].to_location);
            if (j_dest_parent != dest_parent) {
                continue;
            }

            // Check ALL moves targeting same destination (convoyed or not)
            // Skip disrupted convoys - they're not competing
            if (attempts[j].is_convoyed && attempts[j].convoy_disrupted) {
                continue;
            }

            if (attempts[j].attack_strength > max_str) {
                is_strongest = 0;
                break;
            } else if (attempts[j].attack_strength == max_str) {
                num_at_max++;
            }
        }

        // Only succeed if we're the unique strongest (no ties)
        if (is_strongest && num_at_max == 1) {
            attempt->can_move = 1;

            // If we're dislodging a defender, record dislodgement now
            if (defender_idx >= 0 && !attempts[defender_idx].can_move) {
                MoveAttempt* defender = &attempts[defender_idx];

                // Check if already dislodged
                int already_dislodged = 0;
                for (int d = 0; d < game->num_dislodged; d++) {
                    if (game->dislodged[d].power_id == defender->unit_power &&
                        game->dislodged[d].from_location == defender->from_location) {
                        already_dislodged = 1;
                        break;
                    }
                }

                if (!already_dislodged && game->num_dislodged < MAX_UNITS) {
                    DislodgedUnit* dislodged = &game->dislodged[game->num_dislodged++];
                    dislodged->type = defender->unit_type;
                    dislodged->power_id = defender->unit_power;
                    dislodged->from_location = defender->from_location;
                    dislodged->dislodged_by_location = attempt->from_location;
                    dislodged->attacker_used_convoy = 1;  // Convoyed attack
                    dislodged->num_possible_retreats = 0;
                }
            }
        }
    }

}

static void apply_successful_moves(GameState* game, MoveAttempt* attempts, int num_attempts,
                                     SupportOrder* supports, int num_supports) {
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

        // For armies, always use parent location (armies ignore coasts) - DATC 6.B.12
        int dest_loc = attempt->to_location;
        if (attempt->unit_type == UNIT_ARMY) {
            dest_loc = get_parent_location(game->map, attempt->to_location);
        }

        unit->location = dest_loc;
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

    // Record all locations that had combat (for retreat exclusion)
    // A location had combat if it was attacked by one or more units
    // IMPORTANT: Only count attacks from units that were NOT dislodged (DATC 6.H.9)
    // IMPORTANT: Disrupted convoys do NOT contest destination (DATC 6.F.7-8)
    for (int i = 0; i < num_attempts; i++) {
        MoveAttempt* attempt = &attempts[i];

        if (attempt->to_location < 0) {
            continue;  // Not a move order
        }

        // DATC 6.F.7-8: Disrupted convoys don't contest destination
        // The convoyed move never actually reached the destination
        if (attempt->is_convoyed && attempt->convoy_disrupted) {
            continue;  // Disrupted convoy doesn't make destination contested
        }

        // Check if the attacker was dislodged - if so, don't count this as combat
        // (DATC 6.H.9: "DISLODGED UNIT WILL NOT MAKE ATTACKERS AREA CONTESTED")
        int attacker_was_dislodged = 0;
        for (int d = 0; d < game->num_dislodged; d++) {
            if (game->dislodged[d].power_id == attempt->unit_power &&
                game->dislodged[d].from_location == attempt->from_location) {
                attacker_was_dislodged = 1;
                break;
            }
        }

        if (attacker_was_dislodged) {
            continue;  // Dislodged attackers don't make destination contested
        }

        // Check if this location already recorded
        int already_recorded = 0;
        for (int c = 0; c < game->num_combats; c++) {
            if (game->combats[c].location == attempt->to_location) {
                already_recorded = 1;
                break;
            }
        }

        if (!already_recorded && game->num_combats < MAX_LOCATIONS) {
            // Record this location as having combat
            Combat* combat = &game->combats[game->num_combats++];
            combat->location = attempt->to_location;
            combat->attack_strength = attempt->attack_strength;
            combat->attacker_location = attempt->from_location;
            combat->attacker_power = attempt->unit_power;
            combat->defender_power = -1;  // Will be set if needed
            combat->successful = attempt->can_move ? 1 : 0;
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

        if (order->type == ORDER_MOVE) {
            // For moves, set the move result (bounce/success/void/no_convoy)
            // Dislodgement will be added by adapter separately
            if (attempt->can_move && attempt->to_location != -1) {
                order->result = RESULT_SUCCESS;
            } else if (attempt->to_location == -1) {
                // Invalid move that was converted to hold
                order->result = RESULT_VOID;
            } else if (attempt->is_convoyed && attempt->convoy_disrupted) {
                // Convoyed move failed because convoy was disrupted
                order->result = RESULT_NO_CONVOY;
            } else {
                // Move failed - bounced
                order->result = RESULT_BOUNCE;
            }
        } else if (order->type == ORDER_HOLD) {
            // Hold orders that weren't dislodged succeeded
            order->result = was_dislodged ? RESULT_DISLODGED : RESULT_SUCCESS;
        } else if (order->type == ORDER_CONVOY) {
            // Convoy orders: always check convoy result (dislodged status tracked separately)
            // Convoy orders: check if valid and if convoy was used/disrupted
            if (!attempt->is_valid) {
                // Invalid convoy order (bad syntax or impossible convoy)
                order->result = RESULT_VOID;
            } else {
                // Check if there's a unit actually using this convoy
                int convoy_used = 0;
                int convoy_disrupted = 0;

                // Look for a move attempt using this convoy
                int convoy_paradox = 0;
                for (int check_i = 0; check_i < num_attempts; check_i++) {
                    MoveAttempt* check_attempt = &attempts[check_i];
                    if (check_attempt->is_convoyed &&
                        check_attempt->from_location == order->target_unit_location &&
                        check_attempt->to_location == order->dest_location) {
                        convoy_used = 1;
                        if (check_attempt->convoy_disrupted) {
                            convoy_disrupted = 1;
                            if (check_attempt->convoy_paradox) {
                                convoy_paradox = 1;
                            }
                        }
                        break;
                    }
                }

                if (!convoy_used) {
                    // Convoy not used - check if the target move EXISTS
                    // If move exists but doesn't use convoy (e.g., adjacent land route) → NO_CONVOY
                    // If move doesn't exist at all → VOID
                    int move_exists = 0;
                    for (int check_i = 0; check_i < num_attempts; check_i++) {
                        MoveAttempt* check_attempt = &attempts[check_i];
                        if (check_attempt->from_location == order->target_unit_location &&
                            check_attempt->to_location == order->dest_location) {
                            move_exists = 1;
                            break;
                        }
                    }
                    order->result = move_exists ? RESULT_NO_CONVOY : RESULT_VOID;
                } else if (convoy_disrupted) {
                    // Convoy was used but disrupted
                    // DISRUPTED if due to paradox (Szykman rule), NO_CONVOY if dislodged
                    order->result = convoy_paradox ? RESULT_DISRUPTED : RESULT_NO_CONVOY;
                } else {
                    // Convoy used - was THIS fleet part of the convoy path?
                    // DATC 6.G.6: If multiple fleets offer convoy and move uses different route,
                    // the unused fleet gets NO_CONVOY, not SUCCESS
                    // Note: If the army bounced, convoy was still used (SUCCESS)
                    int fleet_on_path = 0;
                    int fleet_loc = attempt->from_location;

                    // Find the convoyed move (even if it bounced) and check if this fleet was on the path
                    for (int check_i = 0; check_i < num_attempts; check_i++) {
                        MoveAttempt* check_attempt = &attempts[check_i];
                        if (check_attempt->is_convoyed &&
                            check_attempt->from_location == order->target_unit_location &&
                            check_attempt->to_location == order->dest_location) {
                            // Found the convoyed move - if it succeeded, check path necessity
                            // If it bounced, the convoy was still valid (SUCCESS)
                            if (!check_attempt->can_move) {
                                // Army bounced - convoy was used, fleet gets SUCCESS
                                fleet_on_path = 1;
                                break;
                            }
                            // Check if this fleet is on a valid convoy path
                            // Collect all convoying fleets for this move
                            int convoying_fleets[MAX_LOCATIONS];
                            int num_convoying = 0;

                            for (int fp = 0; fp < MAX_POWERS; fp++) {
                                Power* fleet_power = &game->powers[fp];
                                for (int fo = 0; fo < fleet_power->num_orders; fo++) {
                                    Order* fleet_order = &fleet_power->orders[fo];
                                    if (fleet_order->type == ORDER_CONVOY &&
                                        fleet_order->target_unit_location == check_attempt->from_location &&
                                        fleet_order->dest_location == check_attempt->to_location) {
                                        convoying_fleets[num_convoying++] = fleet_order->unit_location;
                                    }
                                }
                            }

                            // Check if this fleet is in the list AND the path still works with only this fleet
                            // (i.e., this fleet is necessary for the path)
                            int this_fleet_in_list = 0;
                            for (int cf = 0; cf < num_convoying; cf++) {
                                if (convoying_fleets[cf] == fleet_loc) {
                                    this_fleet_in_list = 1;
                                    break;
                                }
                            }

                            if (this_fleet_in_list) {
                                // Check if removing this fleet breaks the path
                                int other_fleets[MAX_LOCATIONS];
                                int num_other = 0;
                                for (int cf = 0; cf < num_convoying; cf++) {
                                    if (convoying_fleets[cf] != fleet_loc) {
                                        other_fleets[num_other++] = convoying_fleets[cf];
                                    }
                                }

                                if (num_other == 0 ||
                                    !find_convoy_path(game->map, check_attempt->from_location,
                                                      check_attempt->to_location,
                                                      other_fleets, num_other)) {
                                    // Path doesn't exist without this fleet - fleet is necessary
                                    fleet_on_path = 1;
                                }
                            }
                            break;
                        }
                    }

                    order->result = fleet_on_path ? RESULT_SUCCESS : RESULT_NO_CONVOY;
                }
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

        // DATC 6.D.34: Invalid support is VOID regardless of dislodgement
        // Check validity FIRST before checking dislodgement
        if (!support->is_valid) {
            order->result = RESULT_VOID;
            // Note: If also dislodged, adapter will add 'dislodged' from dislodged list
        } else if (was_dislodged) {
            // Dislodged support is cut (DATC 6.D.17)
            // The adapter will add 'dislodged' automatically from the dislodged units list
            order->result = RESULT_CUT;
            support->is_cut = 1;  // Mark as cut for strength calculations
        } else if (support->is_cut) {
            order->result = RESULT_CUT;
        } else {
            // Check if the supported unit is actually making the move being supported
            int move_exists = 0;
            for (int check_i = 0; check_i < num_attempts; check_i++) {
                MoveAttempt* check_attempt = &attempts[check_i];
                // For SUPPORT_MOVE, check if unit at supported_location is moving to destination
                if (order->type == ORDER_SUPPORT_MOVE) {
                    // Compare parent locations to handle coast variants (DATC 6.B.7)
                    int from_match = (get_parent_location(game->map, check_attempt->from_location) ==
                                     get_parent_location(game->map, support->supported_location));
                    int to_match = (get_parent_location(game->map, check_attempt->to_location) ==
                                   get_parent_location(game->map, support->destination));
                    if (from_match && to_match) {
                        move_exists = 1;
                        break;
                    }
                } else {  // SUPPORT_HOLD
                    // For hold support, check if unit exists at supported location
                    // DATC 6.D.28-32: Impossible moves are treated as holds, so support is valid
                    int loc_match = (get_parent_location(game->map, check_attempt->from_location) ==
                                    get_parent_location(game->map, support->supported_location));
                    if (loc_match) {
                        move_exists = 1;
                        break;
                    }
                }
            }

            if (!move_exists) {
                // Supporting a move that doesn't exist (unit is holding or moving elsewhere)
                order->result = RESULT_VOID;
            } else {
                // Check if the supported move was a convoyed move that was disrupted
                int convoy_failed = 0;
                for (int check_i = 0; check_i < num_attempts; check_i++) {
                    MoveAttempt* check_attempt = &attempts[check_i];
                    if (order->type == ORDER_SUPPORT_MOVE) {
                        int from_match = (get_parent_location(game->map, check_attempt->from_location) ==
                                         get_parent_location(game->map, support->supported_location));
                        int to_match = (get_parent_location(game->map, check_attempt->to_location) ==
                                       get_parent_location(game->map, support->destination));
                        if (from_match && to_match && check_attempt->is_convoyed && check_attempt->convoy_disrupted) {
                            convoy_failed = 1;
                            break;
                        }
                    }
                }
                if (convoy_failed) {
                    // Support for a convoyed move that failed due to disruption
                    order->result = RESULT_NO_CONVOY;
                } else {
                    order->result = RESULT_SUCCESS;
                }
            }
    }
  }
}

static void save_results_and_finalize(GameState* game, MoveAttempt* attempts, int num_attempts,
                                       SupportOrder* supports, int num_supports) {
    // Save results to persistent storage before they get cleared
    for (int p = 0; p < MAX_POWERS; p++) {
        Power* power = &game->powers[p];
        game->last_num_orders[p] = power->num_orders;
        for (int o = 0; o < power->num_orders; o++) {
            game->last_results[p][o] = power->orders[o].result;
            game->last_unit_locations[p][o] = power->orders[o].unit_location;  // Save for coast normalization
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
            // (DATC 6.H.16: If one coast is contested, other coasts are also unavailable)
            int had_combat = 0;
            for (int c = 0; c < game->num_combats; c++) {
                int combat_loc = game->combats[c].location;

                // Direct match
                if (combat_loc == adj_loc) {
                    had_combat = 1;
                    break;
                }

                // Check if adj_loc and combat_loc are different coasts of same territory
                const char* adj_name = game->map->locations[adj_loc].name;
                const char* combat_name = game->map->locations[combat_loc].name;
                const char* adj_slash = strchr(adj_name, '/');
                const char* combat_slash = strchr(combat_name, '/');

                if (adj_slash && combat_slash) {
                    // Both have coasts - check if same base territory
                    int adj_base_len = adj_slash - adj_name;
                    int combat_base_len = combat_slash - combat_name;

                    if (adj_base_len == combat_base_len &&
                        strncmp(adj_name, combat_name, adj_base_len) == 0) {
                        // Same territory - if one coast contested, all coasts unavailable
                        had_combat = 1;
                        break;
                    }
                }
            }

            if (had_combat) {
                continue;  // Skip contested location
            }

            // Cannot do "coastal crawl" - cannot retreat to other coast of attacker's origin
            // (DATC 6.H.15: "NO COASTAL CRAWL IN RETREAT")
            // Check if adj_loc is a different coast of the same territory as attacker_loc
            const char* adj_name = game->map->locations[adj_loc].name;
            const char* attacker_name = game->map->locations[attacker_loc].name;

            // Check if both are coasts (contain '/') and have same base territory
            int is_coastal_crawl = 0;
            const char* adj_slash = strchr(adj_name, '/');
            const char* attacker_slash = strchr(attacker_name, '/');

            if (adj_slash && attacker_slash) {
                // Both have coasts - check if same base territory but different coasts
                int base_len = adj_slash - adj_name;
                int attacker_base_len = attacker_slash - attacker_name;

                if (base_len == attacker_base_len &&
                    strncmp(adj_name, attacker_name, base_len) == 0 &&
                    strcmp(adj_slash, attacker_slash) != 0) {
                    // Same base territory, different coasts - this is coastal crawl
                    is_coastal_crawl = 1;
                }
            }

            if (is_coastal_crawl) {
                continue;  // Skip - cannot coastal crawl in retreat
            }

            // Valid retreat destination
            retreat->possible_retreats[retreat->num_possible_retreats].location = adj_loc;
            retreat->num_possible_retreats++;
        }
    }
}

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

    collect_movement_orders(game, attempts, &num_attempts, supports, &num_supports);
    detect_support_cuts(game, attempts, num_attempts, supports, num_supports);
    calculate_strengths(game, attempts, num_attempts, supports, num_supports);
    resolve_conflicts_and_circular(game, attempts, num_attempts, supports, num_supports);

    // After dislodgements are determined, mark dislodged supports as cut and recalculate (DATC 6.D.17)
    int needs_recalc = 0;
    for (int s = 0; s < num_supports; s++) {
        SupportOrder* support = &supports[s];
        if (!support->is_cut) {  // Only check supports not already cut
            for (int d = 0; d < game->num_dislodged; d++) {
                if (game->dislodged[d].power_id == support->supporter_power &&
                    game->dislodged[d].from_location == support->supporter_location) {
                    support->is_cut = 1;
                    needs_recalc = 1;
                    break;
                }
            }
        }
    }

    // Recalculate strengths and re-resolve if any dislodged supports were marked as cut
    if (needs_recalc) {
        // Clear previous dislodgements and reset strengths
        game->num_dislodged = 0;
        for (int i = 0; i < num_attempts; i++) {
            attempts[i].attack_strength = 1;
            attempts[i].defend_strength = 1;
            attempts[i].can_move = 0;  // Reset can_move flags
        }
        calculate_strengths(game, attempts, num_attempts, supports, num_supports);
        resolve_conflicts_and_circular(game, attempts, num_attempts, supports, num_supports);
    }

    apply_successful_moves(game, attempts, num_attempts, supports, num_supports);
    save_results_and_finalize(game, attempts, num_attempts, supports, num_supports);
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

    // First pass: Mark all non-retreat orders as VOID (only retreats and disbands allowed)
    // Also mark retreat/disband orders for non-dislodged units as VOID
    for (int p = 0; p < MAX_POWERS; p++) {
        Power* power = &game->powers[p];
        for (int o = 0; o < power->num_orders; o++) {
            Order* order = &power->orders[o];
            if (order->type != ORDER_RETREAT && order->type != ORDER_DISBAND) {
                // Invalid order type for retreat phase
                order->result = RESULT_VOID;
            } else {
                // Retreat or disband order - check if this unit was actually dislodged
                int unit_dislodged = 0;
                for (int r = 0; r < power->num_retreats; r++) {
                    if (power->retreats[r].from_location == order->unit_location) {
                        unit_dislodged = 1;
                        break;
                    }
                }
                if (!unit_dislodged) {
                    // Retreat/disband order for non-dislodged unit
                    order->result = RESULT_VOID;
                }
            }
        }
    }

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
            game->last_unit_locations[p][o] = power->orders[o].unit_location;
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
                            // Validate unit type for location type
                            if (order->unit_type == UNIT_FLEET) {
                                // Fleet can only be built on coastal/water locations
                                // LOC_LAND: INVALID (can't build fleet on land)
                                // LOC_PORT: INVALID (need coast specification like STP/NC)
                                // LOC_COAST/LOC_WATER: VALID
                                if (loc->type == LOC_LAND || loc->type == LOC_PORT) {
                                    is_valid_build = 0;
                                } else {
                                    is_valid_build = 1;
                                }
                            } else {
                                // Army can be built on land or coastal locations
                                is_valid_build = 1;
                            }
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
        // DATC 6.J: Remove units in order of:
        // 1. Distance from home centers (furthest first)
        // 2. Fleets before armies at same distance
        // 3. Alphabetical by location name as tiebreaker
        while (diff > 0 && power->num_units > 0) {
            int unit_to_remove = -1;
            int max_distance = -1;
            int is_fleet = 0;
            const char* unit_name = "";

            for (int u = 0; u < power->num_units; u++) {
                Unit* unit = &power->units[u];
                int unit_loc = unit->location;

                // Calculate distance to nearest home center
                // Use BFS respecting unit type movement restrictions
                int min_distance = 999;

                // BFS setup
                int queue[MAX_LOCATIONS];
                int distances[MAX_LOCATIONS];
                for (int i = 0; i < MAX_LOCATIONS; i++) distances[i] = -1;
                int queue_start = 0, queue_end = 0;

                // Start from unit location
                queue[queue_end++] = unit_loc;
                distances[unit_loc] = 0;

                // Also start from parent location for split coasts
                int parent_loc = get_parent_location(game->map, unit_loc);
                if (parent_loc != unit_loc && distances[parent_loc] < 0) {
                    queue[queue_end++] = parent_loc;
                    distances[parent_loc] = 0;
                }

                // BFS
                while (queue_start < queue_end && min_distance == 999) {
                    int curr = queue[queue_start++];
                    int curr_dist = distances[curr];

                    // Check if this is a home center
                    int curr_parent = get_parent_location(game->map, curr);
                    for (int h = 0; h < game->map->num_homes[p]; h++) {
                        int home = game->map->home_centers[p][h];
                        int home_parent = get_parent_location(game->map, home);
                        if (curr_parent == home_parent) {
                            min_distance = curr_dist;
                            break;
                        }
                    }

                    if (min_distance < 999) break;

                    // Expand to adjacent locations
                    Location* loc = &game->map->locations[curr];
                    for (int a = 0; a < loc->num_adjacent; a++) {
                        int adj = loc->adjacencies[a];
                        if (adj < 0 || distances[adj] >= 0) continue;

                        // For fleets: use can_move (only water/coast)
                        // For armies: allow all adjacencies including sea (convoy distance)
                        // DATC 4.D.8 / 6.J.10-11: armies can cross sea for distance calc
                        int can_go = 0;
                        if (unit->type == UNIT_FLEET) {
                            can_go = can_move(game->map, unit->type, curr, adj);
                        } else {
                            // Army: allow land, coast, and water (convoy)
                            can_go = 1;
                        }

                        if (can_go) {
                            distances[adj] = curr_dist + 1;
                            queue[queue_end++] = adj;
                        }
                    }
                }

                // Compare with current max
                const char* this_name = game->map->locations[unit_loc].name;
                int this_is_fleet = (unit->type == UNIT_FLEET) ? 1 : 0;

                int is_worse = 0;  // Whether this unit should be removed before current best
                if (min_distance > max_distance) {
                    is_worse = 1;
                } else if (min_distance == max_distance) {
                    // Tie on distance - fleet takes precedence over army
                    if (this_is_fleet > is_fleet) {
                        is_worse = 1;
                    } else if (this_is_fleet == is_fleet) {
                        // Same type - alphabetical order
                        if (strcmp(this_name, unit_name) < 0) {
                            is_worse = 1;
                        }
                    }
                }

                if (is_worse) {
                    unit_to_remove = u;
                    max_distance = min_distance;
                    is_fleet = this_is_fleet;
                    unit_name = this_name;
                }
            }

            // Remove the selected unit
            if (unit_to_remove >= 0) {
                for (int i = unit_to_remove; i < power->num_units - 1; i++) {
                    power->units[i] = power->units[i + 1];
                }
                power->num_units--;
                diff--;
            } else {
                break;  // Safety: no unit found
            }
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
            game->last_unit_locations[p][o] = power->orders[o].unit_location;
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
