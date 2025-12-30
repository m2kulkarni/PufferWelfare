#include "diplomacy.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

void init_standard_map(Map* map) {
    memset(map, 0, sizeof(Map));

    // Power names
    strcpy(map->power_names[0], "AUSTRIA");
    strcpy(map->power_names[1], "ENGLAND");
    strcpy(map->power_names[2], "FRANCE");
    strcpy(map->power_names[3], "GERMANY");
    strcpy(map->power_names[4], "ITALY");
    strcpy(map->power_names[5], "RUSSIA");
    strcpy(map->power_names[6], "TURKEY");

    // Power abbreviations
    strcpy(map->power_abbrev[0], "A");
    strcpy(map->power_abbrev[1], "E");
    strcpy(map->power_abbrev[2], "F");
    strcpy(map->power_abbrev[3], "G");
    strcpy(map->power_abbrev[4], "I");
    strcpy(map->power_abbrev[5], "R");
    strcpy(map->power_abbrev[6], "T");

    // Initialize all locations (generated from standard.map)
    map->num_locations = 82;  // 76 base + 6 coast variants (no lowercase duplicates)

    // Initialize new coast fields for all locations (default: not a coast variant)
    for (int i = 0; i < MAX_LOCATIONS; i++) {
        map->locations[i].parent_location = -1;
        map->locations[i].coast_type = COAST_NONE;
    }

    // 0: ADR
    strcpy(map->locations[0].name, "ADR");
    map->locations[0].type = LOC_WATER;
    map->locations[0].has_supply_center = 0;
    map->locations[0].owner_power = -1;  // Neutral initially
    map->locations[0].num_adjacent = 5;
    map->locations[0].adjacencies[0] = 2;
    map->locations[0].adjacencies[1] = 4;
    map->locations[0].adjacencies[2] = 29;
    map->locations[0].adjacencies[3] = 64;
    map->locations[0].adjacencies[4] = 70;

    // 1: AEG
    strcpy(map->locations[1].name, "AEG");
    map->locations[1].type = LOC_WATER;
    map->locations[1].has_supply_center = 0;
    map->locations[1].owner_power = -1;  // Neutral initially
    map->locations[1].num_adjacent = 6;
    map->locations[1].adjacencies[0] = 77;  // BUL/SC
    map->locations[1].adjacencies[1] = 18;
    map->locations[1].adjacencies[2] = 20;
    map->locations[1].adjacencies[3] = 26;
    map->locations[1].adjacencies[4] = 29;
    map->locations[1].adjacencies[5] = 58;

    // 2: ALB
    strcpy(map->locations[2].name, "ALB");
    map->locations[2].type = LOC_COAST;
    map->locations[2].has_supply_center = 0;
    map->locations[2].owner_power = -1;  // Neutral initially
    map->locations[2].num_adjacent = 5;
    map->locations[2].adjacencies[0] = 0;
    map->locations[2].adjacencies[1] = 26;
    map->locations[2].adjacencies[2] = 29;
    map->locations[2].adjacencies[3] = 54;
    map->locations[2].adjacencies[4] = 64;

    // 3: ANK
    strcpy(map->locations[3].name, "ANK");
    map->locations[3].type = LOC_COAST;
    map->locations[3].has_supply_center = 1;
    map->locations[3].owner_power = -1;  // Neutral initially
    map->locations[3].num_adjacent = 4;
    map->locations[3].adjacencies[0] = 5;
    map->locations[3].adjacencies[1] = 10;
    map->locations[3].adjacencies[2] = 18;
    map->locations[3].adjacencies[3] = 58;

    // 4: APU
    strcpy(map->locations[4].name, "APU");
    map->locations[4].type = LOC_COAST;
    map->locations[4].has_supply_center = 0;
    map->locations[4].owner_power = -1;  // Neutral initially
    map->locations[4].num_adjacent = 5;
    map->locations[4].adjacencies[0] = 0;
    map->locations[4].adjacencies[1] = 29;
    map->locations[4].adjacencies[2] = 42;
    map->locations[4].adjacencies[3] = 51;
    map->locations[4].adjacencies[4] = 70;

    // 5: ARM
    strcpy(map->locations[5].name, "ARM");
    map->locations[5].type = LOC_COAST;
    map->locations[5].has_supply_center = 0;
    map->locations[5].owner_power = -1;  // Neutral initially
    map->locations[5].num_adjacent = 5;
    map->locations[5].adjacencies[0] = 3;
    map->locations[5].adjacencies[1] = 10;
    map->locations[5].adjacencies[2] = 55;
    map->locations[5].adjacencies[3] = 58;
    map->locations[5].adjacencies[4] = 63;

    // 6: BAL
    strcpy(map->locations[6].name, "BAL");
    map->locations[6].type = LOC_WATER;
    map->locations[6].has_supply_center = 0;
    map->locations[6].owner_power = -1;  // Neutral initially
    map->locations[6].num_adjacent = 7;
    map->locations[6].adjacencies[0] = 9;
    map->locations[6].adjacencies[1] = 12;
    map->locations[6].adjacencies[2] = 19;
    map->locations[6].adjacencies[3] = 33;
    map->locations[6].adjacencies[4] = 31;
    map->locations[6].adjacencies[5] = 50;
    map->locations[6].adjacencies[6] = 61;

    // 7: BAR
    strcpy(map->locations[7].name, "BAR");
    map->locations[7].type = LOC_WATER;
    map->locations[7].has_supply_center = 0;
    map->locations[7].owner_power = -1;  // Neutral initially
    map->locations[7].num_adjacent = 3;
    map->locations[7].adjacencies[0] = 45;
    map->locations[7].adjacencies[1] = 44;
    map->locations[7].adjacencies[2] = 80;  // STP/NC

    // 8: BEL
    strcpy(map->locations[8].name, "BEL");
    map->locations[8].type = LOC_COAST;
    map->locations[8].has_supply_center = 1;
    map->locations[8].owner_power = -1;  // Neutral initially
    map->locations[8].num_adjacent = 6;
    map->locations[8].adjacencies[0] = 16;
    map->locations[8].adjacencies[1] = 22;
    map->locations[8].adjacencies[2] = 28;
    map->locations[8].adjacencies[3] = 43;
    map->locations[8].adjacencies[4] = 47;
    map->locations[8].adjacencies[5] = 52;

    // 9: BER
    strcpy(map->locations[9].name, "BER");
    map->locations[9].type = LOC_COAST;
    map->locations[9].has_supply_center = 0;
    map->locations[9].owner_power = -1;  // Neutral initially
    map->locations[9].num_adjacent = 5;
    map->locations[9].adjacencies[0] = 6;
    map->locations[9].adjacencies[1] = 31;
    map->locations[9].adjacencies[2] = 39;
    map->locations[9].adjacencies[3] = 50;
    map->locations[9].adjacencies[4] = 56;

    // 10: BLA
    strcpy(map->locations[10].name, "BLA");
    map->locations[10].type = LOC_WATER;
    map->locations[10].has_supply_center = 0;
    map->locations[10].owner_power = -1;  // Neutral initially
    map->locations[10].num_adjacent = 6;
    map->locations[10].adjacencies[0] = 3;
    map->locations[10].adjacencies[1] = 5;
    map->locations[10].adjacencies[2] = 76;  // BUL/EC
    map->locations[10].adjacencies[3] = 18;
    map->locations[10].adjacencies[4] = 53;
    map->locations[10].adjacencies[5] = 55;

    // 11: BOH
    strcpy(map->locations[11].name, "BOH");
    map->locations[11].type = LOC_LAND;
    map->locations[11].has_supply_center = 0;
    map->locations[11].owner_power = -1;  // Neutral initially
    map->locations[11].num_adjacent = 5;
    map->locations[11].adjacencies[0] = 24;
    map->locations[11].adjacencies[1] = 39;
    map->locations[11].adjacencies[2] = 56;
    map->locations[11].adjacencies[3] = 67;
    map->locations[11].adjacencies[4] = 71;

    // 12: BOT
    strcpy(map->locations[12].name, "BOT");
    map->locations[12].type = LOC_WATER;
    map->locations[12].has_supply_center = 0;
    map->locations[12].owner_power = -1;  // Neutral initially
    map->locations[12].num_adjacent = 5;
    map->locations[12].adjacencies[0] = 6;
    map->locations[12].adjacencies[1] = 23;
    map->locations[12].adjacencies[2] = 33;
    map->locations[12].adjacencies[3] = 81;  // STP/SC
    map->locations[12].adjacencies[4] = 61;

    // 13: BRE
    strcpy(map->locations[13].name, "BRE");
    map->locations[13].type = LOC_COAST;
    map->locations[13].has_supply_center = 1;
    map->locations[13].owner_power = -1;  // Neutral initially
    map->locations[13].num_adjacent = 5;
    map->locations[13].adjacencies[0] = 22;
    map->locations[13].adjacencies[1] = 25;
    map->locations[13].adjacencies[2] = 36;
    map->locations[13].adjacencies[3] = 46;
    map->locations[13].adjacencies[4] = 47;

    // 14: BUD
    strcpy(map->locations[14].name, "BUD");
    map->locations[14].type = LOC_LAND;
    map->locations[14].has_supply_center = 1;
    map->locations[14].owner_power = -1;  // Neutral initially
    map->locations[14].num_adjacent = 5;
    map->locations[14].adjacencies[0] = 24;
    map->locations[14].adjacencies[1] = 53;
    map->locations[14].adjacencies[2] = 54;
    map->locations[14].adjacencies[3] = 64;
    map->locations[14].adjacencies[4] = 71;

    // 15: BUL (parent location with full land adjacencies)
    strcpy(map->locations[15].name, "BUL");
    map->locations[15].type = LOC_PORT;  // PORT allows both army and fleet
    map->locations[15].has_supply_center = 1;
    map->locations[15].owner_power = -1;  // Neutral initially
    map->locations[15].num_adjacent = 6;
    map->locations[15].adjacencies[0] = 1;   // AEG
    map->locations[15].adjacencies[1] = 10;  // BLA
    map->locations[15].adjacencies[2] = 18;  // CON
    map->locations[15].adjacencies[3] = 26;  // GRE
    map->locations[15].adjacencies[4] = 53;  // RUM
    map->locations[15].adjacencies[5] = 54;  // SER

    // 16: BUR
    strcpy(map->locations[16].name, "BUR");
    map->locations[16].type = LOC_LAND;
    map->locations[16].has_supply_center = 0;
    map->locations[16].owner_power = -1;  // Neutral initially
    map->locations[16].num_adjacent = 8;
    map->locations[16].adjacencies[0] = 8;
    map->locations[16].adjacencies[1] = 25;
    map->locations[16].adjacencies[2] = 52;
    map->locations[16].adjacencies[3] = 37;
    map->locations[16].adjacencies[4] = 39;
    map->locations[16].adjacencies[5] = 46;
    map->locations[16].adjacencies[6] = 47;
    map->locations[16].adjacencies[7] = 62;

    // 17: CLY
    strcpy(map->locations[17].name, "CLY");
    map->locations[17].type = LOC_COAST;
    map->locations[17].has_supply_center = 0;
    map->locations[17].owner_power = -1;  // Neutral initially
    map->locations[17].num_adjacent = 4;
    map->locations[17].adjacencies[0] = 21;
    map->locations[17].adjacencies[1] = 34;
    map->locations[17].adjacencies[2] = 41;
    map->locations[17].adjacencies[3] = 44;

    // 18: CON
    strcpy(map->locations[18].name, "CON");
    map->locations[18].type = LOC_COAST;
    map->locations[18].has_supply_center = 1;
    map->locations[18].owner_power = -1;  // Neutral initially
    map->locations[18].num_adjacent = 7;
    map->locations[18].adjacencies[0] = 1;
    map->locations[18].adjacencies[1] = 15;  // BUL (parent)
    map->locations[18].adjacencies[2] = 76;  // BUL/EC
    map->locations[18].adjacencies[3] = 77;  // BUL/SC
    map->locations[18].adjacencies[4] = 10;
    map->locations[18].adjacencies[5] = 3;
    map->locations[18].adjacencies[6] = 58;

    // 19: DEN
    strcpy(map->locations[19].name, "DEN");
    map->locations[19].type = LOC_COAST;
    map->locations[19].has_supply_center = 1;
    map->locations[19].owner_power = -1;  // Neutral initially
    map->locations[19].num_adjacent = 6;
    map->locations[19].adjacencies[0] = 6;
    map->locations[19].adjacencies[1] = 27;
    map->locations[19].adjacencies[2] = 31;
    map->locations[19].adjacencies[3] = 43;
    map->locations[19].adjacencies[4] = 57;
    map->locations[19].adjacencies[5] = 61;

    // 20: EAS
    strcpy(map->locations[20].name, "EAS");
    map->locations[20].type = LOC_WATER;
    map->locations[20].has_supply_center = 0;
    map->locations[20].owner_power = -1;  // Neutral initially
    map->locations[20].num_adjacent = 4;
    map->locations[20].adjacencies[0] = 1;
    map->locations[20].adjacencies[1] = 29;
    map->locations[20].adjacencies[2] = 58;
    map->locations[20].adjacencies[3] = 63;

    // 21: EDI
    strcpy(map->locations[21].name, "EDI");
    map->locations[21].type = LOC_COAST;
    map->locations[21].has_supply_center = 0;
    map->locations[21].owner_power = -1;  // Neutral initially
    map->locations[21].num_adjacent = 5;
    map->locations[21].adjacencies[0] = 17;
    map->locations[21].adjacencies[1] = 34;
    map->locations[21].adjacencies[2] = 43;
    map->locations[21].adjacencies[3] = 44;
    map->locations[21].adjacencies[4] = 75;

    // 22: ENG
    strcpy(map->locations[22].name, "ENG");
    map->locations[22].type = LOC_WATER;
    map->locations[22].has_supply_center = 0;
    map->locations[22].owner_power = -1;  // Neutral initially
    map->locations[22].num_adjacent = 8;
    map->locations[22].adjacencies[0] = 8;
    map->locations[22].adjacencies[1] = 13;
    map->locations[22].adjacencies[2] = 30;
    map->locations[22].adjacencies[3] = 32;
    map->locations[22].adjacencies[4] = 36;
    map->locations[22].adjacencies[5] = 43;
    map->locations[22].adjacencies[6] = 47;
    map->locations[22].adjacencies[7] = 72;

    // 23: FIN
    strcpy(map->locations[23].name, "FIN");
    map->locations[23].type = LOC_COAST;
    map->locations[23].has_supply_center = 0;
    map->locations[23].owner_power = -1;  // Neutral initially
    map->locations[23].num_adjacent = 5;
    map->locations[23].adjacencies[0] = 12;
    map->locations[23].adjacencies[1] = 45;
    map->locations[23].adjacencies[2] = 60;  // STP (parent)
    map->locations[23].adjacencies[3] = 81;  // STP/SC
    map->locations[23].adjacencies[4] = 61;

    // 24: GAL
    strcpy(map->locations[24].name, "GAL");
    map->locations[24].type = LOC_LAND;
    map->locations[24].has_supply_center = 0;
    map->locations[24].owner_power = -1;  // Neutral initially
    map->locations[24].num_adjacent = 7;
    map->locations[24].adjacencies[0] = 11;
    map->locations[24].adjacencies[1] = 14;
    map->locations[24].adjacencies[2] = 53;
    map->locations[24].adjacencies[3] = 56;
    map->locations[24].adjacencies[4] = 69;
    map->locations[24].adjacencies[5] = 71;
    map->locations[24].adjacencies[6] = 73;

    // 25: GAS
    strcpy(map->locations[25].name, "GAS");
    map->locations[25].type = LOC_COAST;
    map->locations[25].has_supply_center = 0;
    map->locations[25].owner_power = -1;  // Neutral initially
    map->locations[25].num_adjacent = 7;
    map->locations[25].adjacencies[0] = 16;
    map->locations[25].adjacencies[1] = 13;
    map->locations[25].adjacencies[2] = 36;
    map->locations[25].adjacencies[3] = 37;
    map->locations[25].adjacencies[4] = 46;
    map->locations[25].adjacencies[5] = 59;  // SPA (parent)
    map->locations[25].adjacencies[6] = 78;  // SPA/NC

    // 26: GRE
    strcpy(map->locations[26].name, "GRE");
    map->locations[26].type = LOC_COAST;
    map->locations[26].has_supply_center = 1;
    map->locations[26].owner_power = -1;  // Neutral initially
    map->locations[26].num_adjacent = 6;
    map->locations[26].adjacencies[0] = 1;
    map->locations[26].adjacencies[1] = 2;
    map->locations[26].adjacencies[2] = 15;  // BUL (parent)
    map->locations[26].adjacencies[3] = 77;  // BUL/SC
    map->locations[26].adjacencies[4] = 29;
    map->locations[26].adjacencies[5] = 54;

    // 27: HEL
    strcpy(map->locations[27].name, "HEL");
    map->locations[27].type = LOC_WATER;
    map->locations[27].has_supply_center = 0;
    map->locations[27].owner_power = -1;  // Neutral initially
    map->locations[27].num_adjacent = 4;
    map->locations[27].adjacencies[0] = 19;
    map->locations[27].adjacencies[1] = 28;
    map->locations[27].adjacencies[2] = 31;
    map->locations[27].adjacencies[3] = 43;

    // 28: HOL
    strcpy(map->locations[28].name, "HOL");
    map->locations[28].type = LOC_COAST;
    map->locations[28].has_supply_center = 1;
    map->locations[28].owner_power = -1;  // Neutral initially
    map->locations[28].num_adjacent = 5;
    map->locations[28].adjacencies[0] = 8;
    map->locations[28].adjacencies[1] = 27;
    map->locations[28].adjacencies[2] = 31;
    map->locations[28].adjacencies[3] = 43;
    map->locations[28].adjacencies[4] = 52;

    // 29: ION
    strcpy(map->locations[29].name, "ION");
    map->locations[29].type = LOC_WATER;
    map->locations[29].has_supply_center = 0;
    map->locations[29].owner_power = -1;  // Neutral initially
    map->locations[29].num_adjacent = 9;
    map->locations[29].adjacencies[0] = 0;
    map->locations[29].adjacencies[1] = 1;
    map->locations[29].adjacencies[2] = 2;
    map->locations[29].adjacencies[3] = 4;
    map->locations[29].adjacencies[4] = 20;
    map->locations[29].adjacencies[5] = 26;
    map->locations[29].adjacencies[6] = 42;
    map->locations[29].adjacencies[7] = 65;
    map->locations[29].adjacencies[8] = 68;

    // 30: IRI
    strcpy(map->locations[30].name, "IRI");
    map->locations[30].type = LOC_WATER;
    map->locations[30].has_supply_center = 0;
    map->locations[30].owner_power = -1;  // Neutral initially
    map->locations[30].num_adjacent = 5;
    map->locations[30].adjacencies[0] = 22;
    map->locations[30].adjacencies[1] = 34;
    map->locations[30].adjacencies[2] = 36;
    map->locations[30].adjacencies[3] = 41;
    map->locations[30].adjacencies[4] = 72;

    // 31: KIE
    strcpy(map->locations[31].name, "KIE");
    map->locations[31].type = LOC_COAST;
    map->locations[31].has_supply_center = 0;
    map->locations[31].owner_power = -1;  // Neutral initially
    map->locations[31].num_adjacent = 7;
    map->locations[31].adjacencies[0] = 6;
    map->locations[31].adjacencies[1] = 9;
    map->locations[31].adjacencies[2] = 19;
    map->locations[31].adjacencies[3] = 27;
    map->locations[31].adjacencies[4] = 28;
    map->locations[31].adjacencies[5] = 39;
    map->locations[31].adjacencies[6] = 52;

    // 32: LON
    strcpy(map->locations[32].name, "LON");
    map->locations[32].type = LOC_COAST;
    map->locations[32].has_supply_center = 0;
    map->locations[32].owner_power = -1;  // Neutral initially
    map->locations[32].num_adjacent = 4;
    map->locations[32].adjacencies[0] = 22;
    map->locations[32].adjacencies[1] = 43;
    map->locations[32].adjacencies[2] = 75;
    map->locations[32].adjacencies[3] = 72;

    // 33: LVN
    strcpy(map->locations[33].name, "LVN");
    map->locations[33].type = LOC_COAST;
    map->locations[33].has_supply_center = 0;
    map->locations[33].owner_power = -1;  // Neutral initially
    map->locations[33].num_adjacent = 7;
    map->locations[33].adjacencies[0] = 6;
    map->locations[33].adjacencies[1] = 12;
    map->locations[33].adjacencies[2] = 38;
    map->locations[33].adjacencies[3] = 50;
    map->locations[33].adjacencies[4] = 60;  // STP (parent)
    map->locations[33].adjacencies[5] = 81;  // STP/SC
    map->locations[33].adjacencies[6] = 73;

    // 34: LVP
    strcpy(map->locations[34].name, "LVP");
    map->locations[34].type = LOC_COAST;
    map->locations[34].has_supply_center = 0;
    map->locations[34].owner_power = -1;  // Neutral initially
    map->locations[34].num_adjacent = 6;
    map->locations[34].adjacencies[0] = 17;
    map->locations[34].adjacencies[1] = 21;
    map->locations[34].adjacencies[2] = 30;
    map->locations[34].adjacencies[3] = 41;
    map->locations[34].adjacencies[4] = 72;
    map->locations[34].adjacencies[5] = 75;

    // 35: LYO
    strcpy(map->locations[35].name, "LYO");
    map->locations[35].type = LOC_WATER;
    map->locations[35].has_supply_center = 0;
    map->locations[35].owner_power = -1;  // Neutral initially
    map->locations[35].num_adjacent = 6;
    map->locations[35].adjacencies[0] = 37;
    map->locations[35].adjacencies[1] = 48;
    map->locations[35].adjacencies[2] = 79;  // SPA/SC
    map->locations[35].adjacencies[3] = 66;
    map->locations[35].adjacencies[4] = 68;
    map->locations[35].adjacencies[5] = 74;

    // 36: MAO
    strcpy(map->locations[36].name, "MAO");
    map->locations[36].type = LOC_WATER;
    map->locations[36].has_supply_center = 0;
    map->locations[36].owner_power = -1;  // Neutral initially
    map->locations[36].num_adjacent = 10;
    map->locations[36].adjacencies[0] = 13;
    map->locations[36].adjacencies[1] = 22;
    map->locations[36].adjacencies[2] = 25;
    map->locations[36].adjacencies[3] = 30;
    map->locations[36].adjacencies[4] = 40;
    map->locations[36].adjacencies[5] = 41;
    map->locations[36].adjacencies[6] = 49;
    map->locations[36].adjacencies[7] = 78;  // SPA/NC
    map->locations[36].adjacencies[8] = 79;  // SPA/SC
    map->locations[36].adjacencies[9] = 74;

    // 37: MAR
    strcpy(map->locations[37].name, "MAR");
    map->locations[37].type = LOC_COAST;
    map->locations[37].has_supply_center = 1;
    map->locations[37].owner_power = -1;  // Neutral initially
    map->locations[37].num_adjacent = 7;
    map->locations[37].adjacencies[0] = 16;
    map->locations[37].adjacencies[1] = 25;
    map->locations[37].adjacencies[2] = 35;
    map->locations[37].adjacencies[3] = 48;
    map->locations[37].adjacencies[4] = 59;  // SPA (parent)
    map->locations[37].adjacencies[5] = 79;  // SPA/SC
    map->locations[37].adjacencies[6] = 62;

    // 38: MOS
    strcpy(map->locations[38].name, "MOS");
    map->locations[38].type = LOC_LAND;
    map->locations[38].has_supply_center = 0;
    map->locations[38].owner_power = -1;  // Neutral initially
    map->locations[38].num_adjacent = 5;
    map->locations[38].adjacencies[0] = 33;
    map->locations[38].adjacencies[1] = 55;
    map->locations[38].adjacencies[2] = 60;  // STP
    map->locations[38].adjacencies[3] = 69;
    map->locations[38].adjacencies[4] = 73;

    // 39: MUN
    strcpy(map->locations[39].name, "MUN");
    map->locations[39].type = LOC_LAND;
    map->locations[39].has_supply_center = 0;
    map->locations[39].owner_power = -1;  // Neutral initially
    map->locations[39].num_adjacent = 8;
    map->locations[39].adjacencies[0] = 9;
    map->locations[39].adjacencies[1] = 11;
    map->locations[39].adjacencies[2] = 16;
    map->locations[39].adjacencies[3] = 31;
    map->locations[39].adjacencies[4] = 52;
    map->locations[39].adjacencies[5] = 56;
    map->locations[39].adjacencies[6] = 67;
    map->locations[39].adjacencies[7] = 62;

    // 40: NAF
    strcpy(map->locations[40].name, "NAF");
    map->locations[40].type = LOC_COAST;
    map->locations[40].has_supply_center = 0;
    map->locations[40].owner_power = -1;  // Neutral initially
    map->locations[40].num_adjacent = 3;
    map->locations[40].adjacencies[0] = 36;
    map->locations[40].adjacencies[1] = 65;
    map->locations[40].adjacencies[2] = 74;

    // 41: NAO
    strcpy(map->locations[41].name, "NAO");
    map->locations[41].type = LOC_WATER;
    map->locations[41].has_supply_center = 0;
    map->locations[41].owner_power = -1;  // Neutral initially
    map->locations[41].num_adjacent = 5;
    map->locations[41].adjacencies[0] = 17;
    map->locations[41].adjacencies[1] = 30;
    map->locations[41].adjacencies[2] = 34;
    map->locations[41].adjacencies[3] = 36;
    map->locations[41].adjacencies[4] = 44;

    // 42: NAP
    strcpy(map->locations[42].name, "NAP");
    map->locations[42].type = LOC_COAST;
    map->locations[42].has_supply_center = 1;
    map->locations[42].owner_power = -1;  // Neutral initially
    map->locations[42].num_adjacent = 4;
    map->locations[42].adjacencies[0] = 4;
    map->locations[42].adjacencies[1] = 29;
    map->locations[42].adjacencies[2] = 51;
    map->locations[42].adjacencies[3] = 68;

    // 43: NTH
    strcpy(map->locations[43].name, "NTH");
    map->locations[43].type = LOC_WATER;
    map->locations[43].has_supply_center = 0;
    map->locations[43].owner_power = -1;  // Neutral initially
    map->locations[43].num_adjacent = 11;
    map->locations[43].adjacencies[0] = 8;
    map->locations[43].adjacencies[1] = 19;
    map->locations[43].adjacencies[2] = 21;
    map->locations[43].adjacencies[3] = 22;
    map->locations[43].adjacencies[4] = 32;
    map->locations[43].adjacencies[5] = 27;
    map->locations[43].adjacencies[6] = 28;
    map->locations[43].adjacencies[7] = 45;
    map->locations[43].adjacencies[8] = 44;
    map->locations[43].adjacencies[9] = 57;
    map->locations[43].adjacencies[10] = 75;

    // 44: NWG
    strcpy(map->locations[44].name, "NWG");
    map->locations[44].type = LOC_WATER;
    map->locations[44].has_supply_center = 0;
    map->locations[44].owner_power = -1;  // Neutral initially
    map->locations[44].num_adjacent = 6;
    map->locations[44].adjacencies[0] = 7;
    map->locations[44].adjacencies[1] = 17;
    map->locations[44].adjacencies[2] = 21;
    map->locations[44].adjacencies[3] = 41;
    map->locations[44].adjacencies[4] = 45;
    map->locations[44].adjacencies[5] = 43;

    // 45: NWY
    strcpy(map->locations[45].name, "NWY");
    map->locations[45].type = LOC_COAST;
    map->locations[45].has_supply_center = 1;
    map->locations[45].owner_power = -1;  // Neutral initially
    map->locations[45].num_adjacent = 8;
    map->locations[45].adjacencies[0] = 7;
    map->locations[45].adjacencies[1] = 23;
    map->locations[45].adjacencies[2] = 43;
    map->locations[45].adjacencies[3] = 44;
    map->locations[45].adjacencies[4] = 57;
    map->locations[45].adjacencies[5] = 60;  // STP (parent)
    map->locations[45].adjacencies[6] = 80;  // STP/NC
    map->locations[45].adjacencies[7] = 61;

    // 46: PAR
    strcpy(map->locations[46].name, "PAR");
    map->locations[46].type = LOC_LAND;
    map->locations[46].has_supply_center = 1;
    map->locations[46].owner_power = -1;  // Neutral initially
    map->locations[46].num_adjacent = 4;
    map->locations[46].adjacencies[0] = 16;
    map->locations[46].adjacencies[1] = 13;
    map->locations[46].adjacencies[2] = 25;
    map->locations[46].adjacencies[3] = 47;

    // 47: PIC
    strcpy(map->locations[47].name, "PIC");
    map->locations[47].type = LOC_COAST;
    map->locations[47].has_supply_center = 0;
    map->locations[47].owner_power = -1;  // Neutral initially
    map->locations[47].num_adjacent = 5;
    map->locations[47].adjacencies[0] = 8;
    map->locations[47].adjacencies[1] = 13;
    map->locations[47].adjacencies[2] = 16;
    map->locations[47].adjacencies[3] = 22;
    map->locations[47].adjacencies[4] = 46;

    // 48: PIE
    strcpy(map->locations[48].name, "PIE");
    map->locations[48].type = LOC_COAST;
    map->locations[48].has_supply_center = 0;
    map->locations[48].owner_power = -1;  // Neutral initially
    map->locations[48].num_adjacent = 6;
    map->locations[48].adjacencies[0] = 35;
    map->locations[48].adjacencies[1] = 37;
    map->locations[48].adjacencies[2] = 66;
    map->locations[48].adjacencies[3] = 67;
    map->locations[48].adjacencies[4] = 70;
    map->locations[48].adjacencies[5] = 62;

    // 49: POR
    strcpy(map->locations[49].name, "POR");
    map->locations[49].type = LOC_COAST;
    map->locations[49].has_supply_center = 1;
    map->locations[49].owner_power = -1;  // Neutral initially
    map->locations[49].num_adjacent = 4;
    map->locations[49].adjacencies[0] = 36;
    map->locations[49].adjacencies[1] = 59;  // SPA (parent)
    map->locations[49].adjacencies[2] = 78;  // SPA/NC
    map->locations[49].adjacencies[3] = 79;  // SPA/SC

    // 50: PRU
    strcpy(map->locations[50].name, "PRU");
    map->locations[50].type = LOC_COAST;
    map->locations[50].has_supply_center = 0;
    map->locations[50].owner_power = -1;  // Neutral initially
    map->locations[50].num_adjacent = 5;
    map->locations[50].adjacencies[0] = 6;
    map->locations[50].adjacencies[1] = 9;
    map->locations[50].adjacencies[2] = 33;
    map->locations[50].adjacencies[3] = 56;
    map->locations[50].adjacencies[4] = 73;

    // 51: ROM
    strcpy(map->locations[51].name, "ROM");
    map->locations[51].type = LOC_COAST;
    map->locations[51].has_supply_center = 1;
    map->locations[51].owner_power = -1;  // Neutral initially
    map->locations[51].num_adjacent = 5;
    map->locations[51].adjacencies[0] = 4;
    map->locations[51].adjacencies[1] = 42;
    map->locations[51].adjacencies[2] = 66;
    map->locations[51].adjacencies[3] = 68;
    map->locations[51].adjacencies[4] = 70;

    // 52: RUH
    strcpy(map->locations[52].name, "RUH");
    map->locations[52].type = LOC_LAND;
    map->locations[52].has_supply_center = 0;
    map->locations[52].owner_power = -1;  // Neutral initially
    map->locations[52].num_adjacent = 5;
    map->locations[52].adjacencies[0] = 8;
    map->locations[52].adjacencies[1] = 16;
    map->locations[52].adjacencies[2] = 28;
    map->locations[52].adjacencies[3] = 31;
    map->locations[52].adjacencies[4] = 39;

    // 53: RUM
    strcpy(map->locations[53].name, "RUM");
    map->locations[53].type = LOC_COAST;
    map->locations[53].has_supply_center = 1;
    map->locations[53].owner_power = -1;  // Neutral initially
    map->locations[53].num_adjacent = 8;
    map->locations[53].adjacencies[0] = 10;
    map->locations[53].adjacencies[1] = 14;
    map->locations[53].adjacencies[2] = 15;  // BUL (parent)
    map->locations[53].adjacencies[3] = 76;  // BUL/EC
    map->locations[53].adjacencies[4] = 24;
    map->locations[53].adjacencies[5] = 54;
    map->locations[53].adjacencies[6] = 55;
    map->locations[53].adjacencies[7] = 69;

    // 54: SER
    strcpy(map->locations[54].name, "SER");
    map->locations[54].type = LOC_LAND;
    map->locations[54].has_supply_center = 1;
    map->locations[54].owner_power = -1;  // Neutral initially
    map->locations[54].num_adjacent = 6;
    map->locations[54].adjacencies[0] = 2;
    map->locations[54].adjacencies[1] = 14;
    map->locations[54].adjacencies[2] = 15;  // BUL
    map->locations[54].adjacencies[3] = 26;
    map->locations[54].adjacencies[4] = 53;
    map->locations[54].adjacencies[5] = 64;

    // 55: SEV
    strcpy(map->locations[55].name, "SEV");
    map->locations[55].type = LOC_COAST;
    map->locations[55].has_supply_center = 0;
    map->locations[55].owner_power = -1;  // Neutral initially
    map->locations[55].num_adjacent = 5;
    map->locations[55].adjacencies[0] = 5;
    map->locations[55].adjacencies[1] = 10;
    map->locations[55].adjacencies[2] = 38;
    map->locations[55].adjacencies[3] = 53;
    map->locations[55].adjacencies[4] = 69;

    // 56: SIL
    strcpy(map->locations[56].name, "SIL");
    map->locations[56].type = LOC_LAND;
    map->locations[56].has_supply_center = 0;
    map->locations[56].owner_power = -1;  // Neutral initially
    map->locations[56].num_adjacent = 6;
    map->locations[56].adjacencies[0] = 9;
    map->locations[56].adjacencies[1] = 11;
    map->locations[56].adjacencies[2] = 24;
    map->locations[56].adjacencies[3] = 39;
    map->locations[56].adjacencies[4] = 50;
    map->locations[56].adjacencies[5] = 73;

    // 57: SKA
    strcpy(map->locations[57].name, "SKA");
    map->locations[57].type = LOC_WATER;
    map->locations[57].has_supply_center = 0;
    map->locations[57].owner_power = -1;  // Neutral initially
    map->locations[57].num_adjacent = 4;
    map->locations[57].adjacencies[0] = 19;
    map->locations[57].adjacencies[1] = 45;
    map->locations[57].adjacencies[2] = 43;
    map->locations[57].adjacencies[3] = 61;

    // 58: SMY
    strcpy(map->locations[58].name, "SMY");
    map->locations[58].type = LOC_COAST;
    map->locations[58].has_supply_center = 1;
    map->locations[58].owner_power = -1;  // Neutral initially
    map->locations[58].num_adjacent = 6;
    map->locations[58].adjacencies[0] = 1;
    map->locations[58].adjacencies[1] = 3;
    map->locations[58].adjacencies[2] = 5;
    map->locations[58].adjacencies[3] = 18;
    map->locations[58].adjacencies[4] = 20;
    map->locations[58].adjacencies[5] = 63;

    // 59: SPA (parent location with full land adjacencies)
    strcpy(map->locations[59].name, "SPA");
    map->locations[59].type = LOC_PORT;  // PORT allows both army and fleet
    map->locations[59].has_supply_center = 1;
    map->locations[59].owner_power = -1;  // Neutral initially
    map->locations[59].num_adjacent = 4;
    map->locations[59].adjacencies[0] = 25;  // GAS
    map->locations[59].adjacencies[1] = 35;  // LYO
    map->locations[59].adjacencies[2] = 37;  // MAR
    map->locations[59].adjacencies[3] = 49;  // POR

    // 60: STP (parent location with full land adjacencies)
    strcpy(map->locations[60].name, "STP");
    map->locations[60].type = LOC_PORT;  // PORT allows both army and fleet
    map->locations[60].has_supply_center = 1;  // STP is a supply center (Russia home)
    map->locations[60].owner_power = -1;  // Neutral initially
    map->locations[60].num_adjacent = 6;
    map->locations[60].adjacencies[0] = 7;   // BAR
    map->locations[60].adjacencies[1] = 12;  // BOT
    map->locations[60].adjacencies[2] = 23;  // FIN
    map->locations[60].adjacencies[3] = 33;  // LVN
    map->locations[60].adjacencies[4] = 38;  // MOS
    map->locations[60].adjacencies[5] = 45;  // NWY

    // 61: SWE
    strcpy(map->locations[61].name, "SWE");
    map->locations[61].type = LOC_COAST;
    map->locations[61].has_supply_center = 1;
    map->locations[61].owner_power = -1;  // Neutral initially
    map->locations[61].num_adjacent = 6;
    map->locations[61].adjacencies[0] = 6;
    map->locations[61].adjacencies[1] = 12;
    map->locations[61].adjacencies[2] = 19;
    map->locations[61].adjacencies[3] = 23;
    map->locations[61].adjacencies[4] = 45;
    map->locations[61].adjacencies[5] = 57;

    // 62: SWI
    strcpy(map->locations[62].name, "SWI");
    map->locations[62].type = LOC_COAST;
    map->locations[62].has_supply_center = 0;
    map->locations[62].owner_power = -1;  // Neutral initially
    map->locations[62].num_adjacent = 5;
    map->locations[62].adjacencies[0] = 37;
    map->locations[62].adjacencies[1] = 16;
    map->locations[62].adjacencies[2] = 39;
    map->locations[62].adjacencies[3] = 67;
    map->locations[62].adjacencies[4] = 48;

    // 63: SYR
    strcpy(map->locations[63].name, "SYR");
    map->locations[63].type = LOC_COAST;
    map->locations[63].has_supply_center = 0;
    map->locations[63].owner_power = -1;  // Neutral initially
    map->locations[63].num_adjacent = 3;
    map->locations[63].adjacencies[0] = 5;
    map->locations[63].adjacencies[1] = 20;
    map->locations[63].adjacencies[2] = 58;

    // 64: TRI
    strcpy(map->locations[64].name, "TRI");
    map->locations[64].type = LOC_COAST;
    map->locations[64].has_supply_center = 1;
    map->locations[64].owner_power = -1;  // Neutral initially
    map->locations[64].num_adjacent = 7;
    map->locations[64].adjacencies[0] = 0;
    map->locations[64].adjacencies[1] = 2;
    map->locations[64].adjacencies[2] = 14;
    map->locations[64].adjacencies[3] = 54;
    map->locations[64].adjacencies[4] = 67;
    map->locations[64].adjacencies[5] = 70;
    map->locations[64].adjacencies[6] = 71;

    // 65: TUN
    strcpy(map->locations[65].name, "TUN");
    map->locations[65].type = LOC_COAST;
    map->locations[65].has_supply_center = 1;
    map->locations[65].owner_power = -1;  // Neutral initially
    map->locations[65].num_adjacent = 4;
    map->locations[65].adjacencies[0] = 29;
    map->locations[65].adjacencies[1] = 40;
    map->locations[65].adjacencies[2] = 68;
    map->locations[65].adjacencies[3] = 74;

    // 66: TUS
    strcpy(map->locations[66].name, "TUS");
    map->locations[66].type = LOC_COAST;
    map->locations[66].has_supply_center = 0;
    map->locations[66].owner_power = -1;  // Neutral initially
    map->locations[66].num_adjacent = 5;
    map->locations[66].adjacencies[0] = 35;
    map->locations[66].adjacencies[1] = 48;
    map->locations[66].adjacencies[2] = 51;
    map->locations[66].adjacencies[3] = 68;
    map->locations[66].adjacencies[4] = 70;

    // 67: TYR
    strcpy(map->locations[67].name, "TYR");
    map->locations[67].type = LOC_LAND;
    map->locations[67].has_supply_center = 0;
    map->locations[67].owner_power = -1;  // Neutral initially
    map->locations[67].num_adjacent = 7;
    map->locations[67].adjacencies[0] = 11;
    map->locations[67].adjacencies[1] = 39;
    map->locations[67].adjacencies[2] = 48;
    map->locations[67].adjacencies[3] = 64;
    map->locations[67].adjacencies[4] = 70;
    map->locations[67].adjacencies[5] = 71;
    map->locations[67].adjacencies[6] = 62;

    // 68: TYS
    strcpy(map->locations[68].name, "TYS");
    map->locations[68].type = LOC_WATER;
    map->locations[68].has_supply_center = 0;
    map->locations[68].owner_power = -1;  // Neutral initially
    map->locations[68].num_adjacent = 7;
    map->locations[68].adjacencies[0] = 29;
    map->locations[68].adjacencies[1] = 35;
    map->locations[68].adjacencies[2] = 51;
    map->locations[68].adjacencies[3] = 42;
    map->locations[68].adjacencies[4] = 65;
    map->locations[68].adjacencies[5] = 66;
    map->locations[68].adjacencies[6] = 74;

    // 69: UKR
    strcpy(map->locations[69].name, "UKR");
    map->locations[69].type = LOC_LAND;
    map->locations[69].has_supply_center = 0;
    map->locations[69].owner_power = -1;  // Neutral initially
    map->locations[69].num_adjacent = 5;
    map->locations[69].adjacencies[0] = 24;
    map->locations[69].adjacencies[1] = 38;
    map->locations[69].adjacencies[2] = 53;
    map->locations[69].adjacencies[3] = 55;
    map->locations[69].adjacencies[4] = 73;

    // 70: VEN
    strcpy(map->locations[70].name, "VEN");
    map->locations[70].type = LOC_COAST;
    map->locations[70].has_supply_center = 1;
    map->locations[70].owner_power = -1;  // Neutral initially
    map->locations[70].num_adjacent = 7;
    map->locations[70].adjacencies[0] = 0;
    map->locations[70].adjacencies[1] = 4;
    map->locations[70].adjacencies[2] = 48;
    map->locations[70].adjacencies[3] = 51;
    map->locations[70].adjacencies[4] = 64;
    map->locations[70].adjacencies[5] = 66;
    map->locations[70].adjacencies[6] = 67;

    // 71: VIE
    strcpy(map->locations[71].name, "VIE");
    map->locations[71].type = LOC_LAND;
    map->locations[71].has_supply_center = 1;
    map->locations[71].owner_power = -1;  // Neutral initially
    map->locations[71].num_adjacent = 5;
    map->locations[71].adjacencies[0] = 11;
    map->locations[71].adjacencies[1] = 14;
    map->locations[71].adjacencies[2] = 24;
    map->locations[71].adjacencies[3] = 64;
    map->locations[71].adjacencies[4] = 67;

    // 72: WAL
    strcpy(map->locations[72].name, "WAL");
    map->locations[72].type = LOC_COAST;
    map->locations[72].has_supply_center = 0;
    map->locations[72].owner_power = -1;  // Neutral initially
    map->locations[72].num_adjacent = 5;
    map->locations[72].adjacencies[0] = 22;
    map->locations[72].adjacencies[1] = 30;
    map->locations[72].adjacencies[2] = 32;
    map->locations[72].adjacencies[3] = 34;
    map->locations[72].adjacencies[4] = 75;

    // 73: WAR
    strcpy(map->locations[73].name, "WAR");
    map->locations[73].type = LOC_LAND;
    map->locations[73].has_supply_center = 0;
    map->locations[73].owner_power = -1;  // Neutral initially
    map->locations[73].num_adjacent = 6;
    map->locations[73].adjacencies[0] = 24;
    map->locations[73].adjacencies[1] = 33;
    map->locations[73].adjacencies[2] = 38;
    map->locations[73].adjacencies[3] = 50;
    map->locations[73].adjacencies[4] = 56;
    map->locations[73].adjacencies[5] = 69;

    // 74: WES
    strcpy(map->locations[74].name, "WES");
    map->locations[74].type = LOC_WATER;
    map->locations[74].has_supply_center = 0;
    map->locations[74].owner_power = -1;  // Neutral initially
    map->locations[74].num_adjacent = 6;
    map->locations[74].adjacencies[0] = 36;
    map->locations[74].adjacencies[1] = 35;
    map->locations[74].adjacencies[2] = 40;
    map->locations[74].adjacencies[3] = 79;  // SPA/SC
    map->locations[74].adjacencies[4] = 65;
    map->locations[74].adjacencies[5] = 68;

    // 75: YOR
    strcpy(map->locations[75].name, "YOR");
    map->locations[75].type = LOC_COAST;
    map->locations[75].has_supply_center = 0;
    map->locations[75].owner_power = -1;  // Neutral initially
    map->locations[75].num_adjacent = 5;
    map->locations[75].adjacencies[0] = 21;
    map->locations[75].adjacencies[1] = 32;
    map->locations[75].adjacencies[2] = 34;
    map->locations[75].adjacencies[3] = 43;
    map->locations[75].adjacencies[4] = 72;

    // ===== SPLIT COAST VARIANTS (for fleet-specific movement) =====
    // Parent locations (BUL, SPA, STP) now have full adjacencies above.
    // These coast variants are only needed for fleet moves to specific coasts.

    // 76: BUL/EC (Bulgaria East Coast)
    strcpy(map->locations[76].name, "BUL/EC");
    map->locations[76].type = LOC_COAST;
    map->locations[76].has_supply_center = 0;
    map->locations[76].owner_power = -1;
    map->locations[76].num_adjacent = 3;
    map->locations[76].adjacencies[0] = 10;  // BLA
    map->locations[76].adjacencies[1] = 18;  // CON
    map->locations[76].adjacencies[2] = 53;  // RUM
    map->locations[76].parent_location = 15; // BUL
    map->locations[76].coast_type = COAST_EAST;

    // 77: BUL/SC (Bulgaria South Coast)
    strcpy(map->locations[77].name, "BUL/SC");
    map->locations[77].type = LOC_COAST;
    map->locations[77].has_supply_center = 0;
    map->locations[77].owner_power = -1;
    map->locations[77].num_adjacent = 3;
    map->locations[77].adjacencies[0] = 1;   // AEG
    map->locations[77].adjacencies[1] = 18;  // CON
    map->locations[77].adjacencies[2] = 26;  // GRE
    map->locations[77].parent_location = 15; // BUL
    map->locations[77].coast_type = COAST_SOUTH;

    // 78: SPA/NC (Spain North Coast)
    strcpy(map->locations[78].name, "SPA/NC");
    map->locations[78].type = LOC_COAST;
    map->locations[78].has_supply_center = 0;
    map->locations[78].owner_power = -1;
    map->locations[78].num_adjacent = 3;
    map->locations[78].adjacencies[0] = 25;  // GAS
    map->locations[78].adjacencies[1] = 36;  // MAO
    map->locations[78].adjacencies[2] = 49;  // POR
    map->locations[78].parent_location = 59; // SPA
    map->locations[78].coast_type = COAST_NORTH;

    // 79: SPA/SC (Spain South Coast)
    strcpy(map->locations[79].name, "SPA/SC");
    map->locations[79].type = LOC_COAST;
    map->locations[79].has_supply_center = 0;
    map->locations[79].owner_power = -1;
    map->locations[79].num_adjacent = 5;
    map->locations[79].adjacencies[0] = 35;  // LYO
    map->locations[79].adjacencies[1] = 36;  // MAO
    map->locations[79].adjacencies[2] = 37;  // MAR
    map->locations[79].adjacencies[3] = 49;  // POR
    map->locations[79].adjacencies[4] = 74;  // WES
    map->locations[79].parent_location = 59; // SPA
    map->locations[79].coast_type = COAST_SOUTH;

    // 80: STP/NC (St Petersburg North Coast)
    strcpy(map->locations[80].name, "STP/NC");
    map->locations[80].type = LOC_COAST;
    map->locations[80].has_supply_center = 0;
    map->locations[80].owner_power = -1;
    map->locations[80].num_adjacent = 2;
    map->locations[80].adjacencies[0] = 7;   // BAR
    map->locations[80].adjacencies[1] = 45;  // NWY
    map->locations[80].parent_location = 60; // STP
    map->locations[80].coast_type = COAST_NORTH;

    // 81: STP/SC (St Petersburg South Coast)
    strcpy(map->locations[81].name, "STP/SC");
    map->locations[81].type = LOC_COAST;
    map->locations[81].has_supply_center = 0;
    map->locations[81].owner_power = -1;
    map->locations[81].num_adjacent = 3;
    map->locations[81].adjacencies[0] = 12;  // BOT
    map->locations[81].adjacencies[1] = 23;  // FIN
    map->locations[81].adjacencies[2] = 33;  // LVN
    map->locations[81].parent_location = 60; // STP
    map->locations[81].coast_type = COAST_SOUTH;


    // Home center assignments
    // AUSTRIA
    map->home_centers[0][0] = 14;  // BUD
    map->locations[14].is_home_center = 0;
    map->home_centers[0][1] = 64;  // TRI
    map->locations[64].is_home_center = 0;
    map->home_centers[0][2] = 71;  // VIE
    map->locations[71].is_home_center = 0;
    map->home_centers[0][4] = 14;  // BUD
    map->locations[14].is_home_center = 0;
    map->home_centers[0][6] = 71;  // VIE
    map->locations[71].is_home_center = 0;
    map->home_centers[0][8] = 64;  // TRI
    map->locations[64].is_home_center = 0;
    map->num_homes[0] = 10;

    // FRANCE
    map->home_centers[2][0] = 13;  // BRE
    map->locations[13].is_home_center = 2;
    map->home_centers[2][1] = 37;  // MAR
    map->locations[37].is_home_center = 2;
    map->home_centers[2][2] = 46;  // PAR
    map->locations[46].is_home_center = 2;
    map->home_centers[2][4] = 13;  // BRE
    map->locations[13].is_home_center = 2;
    map->home_centers[2][6] = 37;  // MAR
    map->locations[37].is_home_center = 2;
    map->home_centers[2][8] = 46;  // PAR
    map->locations[46].is_home_center = 2;
    map->num_homes[2] = 10;

    // ITALY
    map->home_centers[4][0] = 42;  // NAP
    map->locations[42].is_home_center = 4;
    map->home_centers[4][1] = 51;  // ROM
    map->locations[51].is_home_center = 4;
    map->home_centers[4][2] = 70;  // VEN
    map->locations[70].is_home_center = 4;
    map->home_centers[4][4] = 42;  // NAP
    map->locations[42].is_home_center = 4;
    map->home_centers[4][6] = 51;  // ROM
    map->locations[51].is_home_center = 4;
    map->home_centers[4][8] = 70;  // VEN
    map->locations[70].is_home_center = 4;
    map->num_homes[4] = 10;

    // TURKEY
    map->home_centers[6][0] = 3;  // ANK
    map->locations[3].is_home_center = 6;
    map->home_centers[6][1] = 18;  // CON
    map->locations[18].is_home_center = 6;
    map->home_centers[6][2] = 58;  // SMY
    map->locations[58].is_home_center = 6;
    map->home_centers[6][4] = 3;  // ANK
    map->locations[3].is_home_center = 6;
    map->home_centers[6][6] = 18;  // CON
    map->locations[18].is_home_center = 6;
    map->home_centers[6][8] = 58;  // SMY
    map->locations[58].is_home_center = 6;
    map->home_centers[6][10] = 8;  // BEL
    map->locations[8].is_home_center = 6;
    map->home_centers[6][11] = 15;  // BUL
    map->locations[15].is_home_center = 6;
    map->home_centers[6][12] = 19;  // DEN
    map->locations[19].is_home_center = 6;
    map->home_centers[6][13] = 26;  // GRE
    map->locations[26].is_home_center = 6;
    map->home_centers[6][14] = 28;  // HOL
    map->locations[28].is_home_center = 6;
    map->home_centers[6][15] = 45;  // NWY
    map->locations[45].is_home_center = 6;
    map->num_homes[6] = 17;

    // The code above sets incorrect home centers. Reset and use proper name-based lookup.
    // Reset all is_home_center to -1
    for (int i = 0; i < map->num_locations; i++) {
        map->locations[i].is_home_center = -1;
    }

    // Helper macro to set home centers
    #define SET_HOME(power_idx, name) \
        do { \
            int idx = find_location_by_name(map, name); \
            if (idx >= 0) { \
                int n = map->num_homes[power_idx]; \
                if (n < MAX_HOME_CENTERS) { \
                    map->home_centers[power_idx][n] = idx; \
                } \
                map->num_homes[power_idx] = n + 1; \
                map->locations[idx].is_home_center = power_idx; \
                map->locations[idx].has_supply_center = 1; \
            } \
        } while(0)

    // Clear counts
    for (int p = 0; p < MAX_POWERS; p++) map->num_homes[p] = 0;

    // Austria
    SET_HOME(0, "BUD"); SET_HOME(0, "TRI"); SET_HOME(0, "VIE");
    // England
    SET_HOME(1, "EDI"); SET_HOME(1, "LON"); SET_HOME(1, "LVP");
    // France
    SET_HOME(2, "BRE"); SET_HOME(2, "MAR"); SET_HOME(2, "PAR");
    // Germany
    SET_HOME(3, "BER"); SET_HOME(3, "KIE"); SET_HOME(3, "MUN");
    // Italy
    SET_HOME(4, "NAP"); SET_HOME(4, "ROM"); SET_HOME(4, "VEN");
    // Russia (4 homes)
    SET_HOME(5, "MOS"); SET_HOME(5, "SEV"); SET_HOME(5, "STP"); SET_HOME(5, "WAR");
    // Turkey
    SET_HOME(6, "ANK"); SET_HOME(6, "CON"); SET_HOME(6, "SMY");

    // Ensure neutral supply centers are marked
    const char* neutrals[] = {"BEL","BUL","DEN","GRE","HOL","NWY","POR","RUM","SER","SPA","SWE","TUN"};
    for (int i = 0; i < 12; i++) {
        int idx = find_location_by_name(map, neutrals[i]);
        if (idx >= 0) map->locations[idx].has_supply_center = 1;
    }

    #undef SET_HOME

    // Build adjacency cache for fast movement lookups
    // Initialize entire cache to 0
    memset(map->adjacency_cache, 0, sizeof(map->adjacency_cache));

    // For each location, mark valid adjacencies based on unit type
    for (int from = 0; from < map->num_locations; from++) {
        Location* from_loc = &map->locations[from];

        for (int adj_idx = 0; adj_idx < from_loc->num_adjacent; adj_idx++) {
            int to = from_loc->adjacencies[adj_idx];
            Location* to_loc = &map->locations[to];

            // ARMY can move: LAND<->LAND, LAND<->COAST, COAST<->COAST, PORT<->*
            // (basically, if neither is pure water)
            if (from_loc->type != LOC_WATER && to_loc->type != LOC_WATER) {
                map->adjacency_cache[UNIT_ARMY][from][to] = 1;
            }

            // FLEET can move: WATER<->WATER, WATER<->COAST
            // Fleets CANNOT move to/from LOC_PORT directly (must use coast variants)
            // For COAST<->COAST, they must share a water neighbor (sea connection)
            if (from_loc->type == LOC_PORT || to_loc->type == LOC_PORT) {
                // Skip - fleets must use specific coast variants, not parent locations
            } else if (from_loc->type == LOC_WATER || to_loc->type == LOC_WATER) {
                // At least one is water - fleets can move
                map->adjacency_cache[UNIT_FLEET][from][to] = 1;
            } else if (from_loc->type == LOC_COAST && to_loc->type == LOC_COAST) {
                // Both are coast - check if they share a water neighbor
                // This prevents land-only routes like ROM<->VEN
                int shares_water = 0;
                for (int i = 0; i < from_loc->num_adjacent && !shares_water; i++) {
                    int neighbor = from_loc->adjacencies[i];
                    if (map->locations[neighbor].type == LOC_WATER) {
                        // Check if this water is also adjacent to destination
                        for (int j = 0; j < to_loc->num_adjacent; j++) {
                            if (to_loc->adjacencies[j] == neighbor) {
                                shares_water = 1;
                                break;
                            }
                        }
                    }
                }
                if (shares_water) {
                    map->adjacency_cache[UNIT_FLEET][from][to] = 1;
                }
            }
        }
    }
}

void free_map(Map* map) {
    // No dynamic allocation in Map currently, but keep for future
    (void)map;  // Suppress unused parameter warning
}
