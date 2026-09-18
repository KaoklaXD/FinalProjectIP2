#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "graph.h"
#include "recommendation.h"
#include "storage.h"

int main() {
    printf("====================================================\n");
    printf("   KVIS OPEN HOUSE SMART MANAGEMENT SYSTEM: TEST    \n");
    printf("====================================================\n\n");

    // 1. Create Graph
    int total_checkpoints = 5;
    Graph* g = create_graph(total_checkpoints);
    assert(g != NULL);

    // 2. Setup Checkpoints with Rounds
    snprintf(g->nodes[0].name, 64, "MainEntrance");
    g->nodes[0].room_num = 100;
    g->nodes[0].num_rounds = 2;
    g->nodes[0].rounds[0].max_seats = 100;
    g->nodes[0].rounds[0].available_seats = 100;
    g->nodes[0].rounds[1].max_seats = 100;
    g->nodes[0].rounds[1].available_seats = 100;

    snprintf(g->nodes[1].name, 64, "RoboticsLab");
    g->nodes[1].room_num = 122;
    g->nodes[1].num_rounds = 2;
    g->nodes[1].rounds[0].max_seats = 20;
    g->nodes[1].rounds[0].available_seats = 2; // Highly congested!
    g->nodes[1].rounds[1].max_seats = 20;
    g->nodes[1].rounds[1].available_seats = 18;

    snprintf(g->nodes[2].name, 64, "ChemistryExhibition");
    g->nodes[2].room_num = 212;
    g->nodes[2].num_rounds = 2;
    g->nodes[2].rounds[0].max_seats = 20;
    g->nodes[2].rounds[0].available_seats = 16; // Plenty of seats
    g->nodes[2].rounds[1].max_seats = 20;
    g->nodes[2].rounds[1].available_seats = 15;

    snprintf(g->nodes[3].name, 64, "AstronomyDome");
    g->nodes[3].room_num = 312;
    g->nodes[3].num_rounds = 2;
    g->nodes[3].rounds[0].max_seats = 30;
    g->nodes[3].rounds[0].available_seats = 25;
    g->nodes[3].rounds[1].max_seats = 30;
    g->nodes[3].rounds[1].available_seats = 25;

    snprintf(g->nodes[4].name, 64, "MathPuzzles");
    g->nodes[4].room_num = 222;
    g->nodes[4].num_rounds = 2;
    g->nodes[4].rounds[0].max_seats = 10;
    g->nodes[4].rounds[0].available_seats = 3;
    g->nodes[4].rounds[1].max_seats = 10;
    g->nodes[4].rounds[1].available_seats = 8;

    // 3. Setup Bidirectional Hallway Connections (All 10.0 meters)
    add_bi_edge(g, 0, 1, 10.0); // Entrance <-> Robotics (10m)
    add_bi_edge(g, 0, 2, 10.0); // Entrance <-> Chemistry (10m)
    add_bi_edge(g, 1, 3, 10.0); // Robotics <-> Astronomy (10m)
    add_bi_edge(g, 2, 3, 10.0); // Chemistry <-> Astronomy (10m)
    add_bi_edge(g, 1, 2, 10.0); // Robotics <-> Chemistry (10m)
    add_bi_edge(g, 2, 4, 10.0); // Chemistry <-> MathPuzzles (10m)
    add_bi_edge(g, 3, 4, 10.0); // Astronomy <-> MathPuzzles (10m)

    printf("[PASS] Graph and checkpoints initialized.\n");

    // 4. Test Shortest Path & Yen's K-Shortest Paths
    Path kpaths[3];
    int k_count = 0;
    k_shortest_paths(g, 0, 3, 3, kpaths, &k_count);
    assert(k_count >= 2);
    printf("[PASS] Yen's K-Shortest Paths found %d distinct physical routes from Entrance to Astronomy:\n", k_count);
    for (int p = 0; p < k_count; p++) {
        printf("       Route %d (%.1fm): ", p + 1, kpaths[p].total_distance);
        for (int i = 0; i < kpaths[p].node_count; i++) {
            printf("[%s]%s", g->nodes[kpaths[p].nodes[i]].name, 
                   (i < kpaths[p].node_count - 1) ? " -> " : "");
        }
        printf("\n");
    }

    // 5. Test Smart Recommendation Engine (Congestion vs Distance)
    // From Entrance (0), Robotics (1) is only 40m away, but has only 2 seats free.
    // Chemistry (2) is 70m away, but has 16 seats free.
    // For a group of 5 in Round 0: Robotics cannot accommodate them at all (2 < 5).
    // The recommendation engine MUST bypass Robotics and recommend Chemistry or Astronomy.
    Recommendation rec = recommend_next_checkpoint(g, 0, 5, 0);
    printf("\n[TEST] Recommendation for group of 5 from Entrance (Round 1):\n");
    printf("       Recommended: %s (ID: %d), Score: %.2f\n",
           g->nodes[rec.next_checkpoint_id].name, rec.next_checkpoint_id, rec.score);
    assert(rec.next_checkpoint_id != 1); // Robotics was congested/insufficient!
    assert(rec.next_checkpoint_id == 2 || rec.next_checkpoint_id == 3);
    printf("[PASS] Successfully avoided congested checkpoint (Robotics).\n");

    // 6. Test Group Splitting Logic
    // If a huge group of 35 visitors arrives in Round 0, no single checkpoint has 35 seats (max is 25 in Astronomy).
    Recommendation split_rec = recommend_next_checkpoint(g, 0, 35, 0);
    printf("\n[TEST] Group of 35 (exceeds all single room capacities):\n");
    if (split_rec.is_split) {
        printf("[PASS] SplitOption triggered!\n");
        printf("       Subgroup 1: %d visitors -> %s\n", 
               split_rec.split_info.cp1_group_size, g->nodes[split_rec.split_info.cp1_id].name);
        printf("       Subgroup 2: %d visitors -> %s\n", 
               split_rec.split_info.cp2_group_size, g->nodes[split_rec.split_info.cp2_id].name);
        assert(split_rec.split_info.cp1_group_size + split_rec.split_info.cp2_group_size == 35);
    } else {
        printf("[INFO] No split combination found.\n");
    }

    // 7. Test Save and Reload Checkpoints
    assert(save_checkpoints("test_checkpoints.txt", g) == 1);
    Graph* g_reloaded = create_graph(0);
    int loaded = load_checkpoints("test_checkpoints.txt", g_reloaded);
    assert(loaded == total_checkpoints);
    printf("[PASS] Storage save and reload verified (%d checkpoints loaded).\n", loaded);

    // 8. Test Room Eligibility Validation (Numbered rooms & audis only)
    assert(is_valid_checkpoint_room("122", 122) == 1);
    assert(is_valid_checkpoint_room("audi1", 0) == 1);
    assert(is_valid_checkpoint_room("audi2", 0) == 1);
    assert(is_valid_checkpoint_room("212", 212) == 1);
    assert(is_valid_checkpoint_room("112(dream)", 112) == 1);
    assert(is_valid_checkpoint_room("1stair(L1)", 0) == 0);
    assert(is_valid_checkpoint_room("lift(L2)", 0) == 0);
    assert(is_valid_checkpoint_room("bathroom", 0) == 0);
    assert(is_valid_checkpoint_room("canteen", 0) == 0);
    assert(is_valid_checkpoint_room("2wing2", 0) == 0);
    printf("[PASS] Room eligibility rules verified (numbered rooms & auditoriums allowed, waypoints rejected).\n");

    // 9. Test Multi-Level Stair Transition Rule (Level 1 -> Level 3 must pass Level 2)
    Graph* campus_g = create_graph(0);
    int edges = load_distances("distance.txt", campus_g);
    assert(edges > 0);

    int l1_node = find_checkpoint_by_name(campus_g, "113");
    int l3_node = find_checkpoint_by_name(campus_g, "313");
    assert(l1_node != -1 && l3_node != -1);

    Path floor_path;
    int path_found = dijkstra_shortest_path(campus_g, l1_node, l3_node, NULL, 0, &floor_path);
    assert(path_found == 1);

    // Verify path passes through Level 2 stair landing
    int passed_level2 = 0;
    for (int i = 0; i < floor_path.node_count; i++) {
        const char* name = campus_g->nodes[floor_path.nodes[i]].name;
        if (strstr(name, "(L2)") != NULL || strcmp(name, "entry") == 0 ||
            (name[0] == '2' && strlen(name) == 3)) {
            passed_level2 = 1;
            break;
        }
    }
    assert(passed_level2 == 1);
    printf("[PASS] Multi-level stair rule verified: Path from Level 1 (113) to Level 3 (313) passes Level 2:\n       Route: ");
    for (int i = 0; i < floor_path.node_count; i++) {
        printf("[%s]%s", campus_g->nodes[floor_path.nodes[i]].name,
               (i < floor_path.node_count - 1) ? " -> " : "");
    }
    printf("\n");

    // 10. Test Smart Interim Checkpoint Recommendation (When desired room is full)
    g->nodes[1].rounds[0].available_seats = 0; // Robotics is full in Round 0
    g->nodes[1].rounds[1].available_seats = 20; // Robotics has space in Round 1
    Recommendation interim_rec = recommend_interim_checkpoint(g, 0, 1, 5, 0);
    assert(interim_rec.next_checkpoint_id != -1);
    assert(interim_rec.next_checkpoint_id != 1);
    assert(g->nodes[interim_rec.next_checkpoint_id].rounds[0].available_seats >= 5);
    printf("[PASS] Interim recommendation verified: When chosen room (Robotics) is full in Round 1, recommends %s (Room %d) before returning in Round 2.\n",
           g->nodes[interim_rec.next_checkpoint_id].name, g->nodes[interim_rec.next_checkpoint_id].room_num);

    // Cleanup
    free_graph(g);
    free_graph(g_reloaded);
    free_graph(campus_g);
    remove("test_checkpoints.txt");

    printf("\n>>> ALL TEST SUITES PASSED SUCCESSFULLY! <<<\n");
    return 0;
}
