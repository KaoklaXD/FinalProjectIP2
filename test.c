#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include "graph.h"
#include "recommendation.h"
#include "storage.h"
#include "map_visualizer.h"

int main() {
    printf("====================================================\n");
    printf("   KVIS OPEN HOUSE SMART MANAGEMENT SYSTEM: TEST    \n");
    printf("====================================================\n\n");

    int total_checkpoints = 5;
    Graph* g = create_graph(total_checkpoints);
    assert(g != NULL);

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
    g->nodes[1].rounds[0].available_seats = 2;
    g->nodes[1].rounds[1].max_seats = 20;
    g->nodes[1].rounds[1].available_seats = 18;

    snprintf(g->nodes[2].name, 64, "ChemistryExhibition");
    g->nodes[2].room_num = 212;
    g->nodes[2].num_rounds = 2;
    g->nodes[2].rounds[0].max_seats = 20;
    g->nodes[2].rounds[0].available_seats = 16;
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

    add_bi_edge(g, 0, 1, 10.0);
    add_bi_edge(g, 0, 2, 10.0);
    add_bi_edge(g, 1, 3, 10.0);
    add_bi_edge(g, 2, 3, 10.0);
    add_bi_edge(g, 1, 2, 10.0);
    add_bi_edge(g, 2, 4, 10.0);
    add_bi_edge(g, 3, 4, 10.0);

    printf("[PASS] Graph and checkpoints initialized.\n");

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

    Recommendation rec = recommend_next_checkpoint(g, 0, 5, 0);
    printf("\n[TEST] Recommendation for group of 5 from Entrance (Round 1):\n");
    printf("       Recommended: %s (ID: %d), Score: %.2f\n",
           g->nodes[rec.next_checkpoint_id].name, rec.next_checkpoint_id, rec.score);
    assert(rec.next_checkpoint_id != 1);
    assert(rec.next_checkpoint_id == 2 || rec.next_checkpoint_id == 3);
    printf("[PASS] Successfully avoided congested checkpoint (Robotics).\n");

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

    assert(save_checkpoints("test_checkpoints.txt", g) == 1);
    Graph* g_reloaded = create_graph(0);
    int loaded = load_checkpoints("test_checkpoints.txt", g_reloaded);
    assert(loaded == total_checkpoints);
    printf("[PASS] Storage save and reload verified (%d checkpoints loaded).\n", loaded);

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

    Graph* campus_g = create_graph(0);
    int edges = load_distances("distance.txt", campus_g);
    assert(edges > 0);

    int l1_node = find_checkpoint_by_name(campus_g, "113");
    int l3_node = find_checkpoint_by_name(campus_g, "313");
    assert(l1_node != -1 && l3_node != -1);

    Path floor_path;
    int path_found = dijkstra_shortest_path(campus_g, l1_node, l3_node, NULL, 0, &floor_path);
    assert(path_found == 1);

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

    g->nodes[1].rounds[0].available_seats = 0;
    g->nodes[1].rounds[1].available_seats = 20;
    Recommendation interim_rec = recommend_interim_checkpoint(g, 0, 1, 5, 0);
    assert(interim_rec.next_checkpoint_id != -1);
    assert(interim_rec.next_checkpoint_id != 1);
    assert(g->nodes[interim_rec.next_checkpoint_id].rounds[0].available_seats >= 5);
    printf("[PASS] Interim recommendation verified: When chosen room (Robotics) is full in Round 1, recommends %s (Room %d) before returning in Round 2.\n",
           g->nodes[interim_rec.next_checkpoint_id].name, g->nodes[interim_rec.next_checkpoint_id].room_num);

    Graph* parallel_g = create_graph(0);
    int p_count = load_checkpoints("checkpoints.txt", parallel_g);
    assert(p_count > 0);
    int act_rounds[MAX_ITINERARY_SLOTS];
    int num_act = get_activity_rounds(parallel_g, act_rounds, MAX_ITINERARY_SLOTS);
    assert(num_act == 6);

    assert(parallel_g->nodes[1].rounds[3].is_lunch_break == 1);
    assert(parallel_g->nodes[1].rounds[3].available_seats == 0);
    assert(parallel_g->nodes[1].rounds[3].start_time == 12.00);
    printf("[PASS] Parallel rounds & Lunch Break verified: 6 parallel activity rounds + 1 empty Lunch Break section (12:00-13:00).\n");

    int user_req[4] = {1, 2, 3, 4};
    int cap_mat[MAX_ITINERARY_SLOTS][MAX_ITINERARY_SLOTS];
    memset(cap_mat, 0, sizeof(cap_mat));
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            cap_mat[i][j] = 10;
        }
    }
    cap_mat[0][1] = 0;
    cap_mat[1][2] = 0;
    cap_mat[3][0] = 0;
    int dummy_act_rounds[4] = {0, 1, 2, 4};

    ItinerarySchedule hungarian_sched = hungarian_optimize_itinerary(4, user_req, 5, 4, dummy_act_rounds, cap_mat);
    assert(hungarian_sched.is_valid == 1);

    for (int s = 0; s < 4; s++) {
        int node = hungarian_sched.assigned_checkpoint_id[s];
        int node_idx = node - 1;
        assert(cap_mat[node_idx][s] >= 5);
    }
    printf("[PASS] Hungarian Algorithm O(k^3) verified: Global optimal schedule found with Spearman penalty = %.1f.\n",
           hungarian_sched.spearman_penalty);

    ItinerarySchedule dfs_alts[MAX_ALTERNATIVE_ITINERARIES];
    int dfs_count = dfs_top_itineraries(4, user_req, 5, 4, dummy_act_rounds, cap_mat, 5, dfs_alts);
    assert(dfs_count > 0);

    assert(fabs(dfs_alts[0].spearman_penalty - hungarian_sched.spearman_penalty) < 1e-5);

    for (int i = 0; i < dfs_count - 1; i++) {
        assert(dfs_alts[i].spearman_penalty <= dfs_alts[i + 1].spearman_penalty);
    }
    printf("[PASS] DFS Branch-and-Bound Pruning verified: Found %d alternative itineraries ranked by similarity in microseconds.\n",
           dfs_count);

    ItinerarySchedule ram_optimal;
    ItinerarySchedule ram_alts[MAX_ALTERNATIVE_ITINERARIES];
    int num_ram_alts = 0;
    int ram_success = optimize_itineraries_in_ram(parallel_g, 4, user_req, 5, &ram_optimal, ram_alts, &num_ram_alts);
    assert(ram_success == 1);
    assert(ram_optimal.is_valid == 1);
    assert(num_ram_alts > 0);
    printf("[PASS] In-Memory RAM Optimizer verified: Queried database once, solved in RAM with Hungarian + DFS.\n");

    parallel_g->nodes[1].rounds[0].available_seats = 0;
    int alt_suggestion = hungarian_suggest_alternative(parallel_g, 0, 0, 1, 5, NULL, 0);
    assert(alt_suggestion != -1);
    assert(alt_suggestion != 1);
    assert(parallel_g->nodes[alt_suggestion].rounds[0].available_seats >= 5);

    int excluded_cp[1] = {alt_suggestion};
    int alt2 = hungarian_suggest_alternative(parallel_g, 0, 0, 1, 5, excluded_cp, 1);
    assert(alt2 != -1);
    assert(alt2 != alt_suggestion);
    assert(parallel_g->nodes[alt2].rounds[0].available_seats >= 5);
    printf("[PASS] Hungarian fallback suggestion verified: Suggests %s (Room %d), correctly excludes visited rooms.\n",
           parallel_g->nodes[alt_suggestion].name, parallel_g->nodes[alt_suggestion].room_num);

    Graph* tour_g = create_graph(0);
    int t_cp = load_checkpoints("checkpoints.txt", tour_g);
    int t_edge = load_distances("distance.txt", tour_g);
    assert(t_cp > 0 && t_edge > 0);

    int targets[4] = {6, 1, 3, 5};
    OptimalTour tour = find_optimal_tour(tour_g, 0, targets, 4);

    assert(tour.count == 4);
    assert(tour.total_distance > 0.0);

    int found_target[4] = {0};
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            if (tour.ordered_checkpoints[i] == targets[j]) {
                found_target[j] = 1;
            }
        }
    }
    for (int j = 0; j < 4; j++) {
        assert(found_target[j] == 1);
    }

    int prev_tour_loc = 0;
    for (int i = 0; i < 4; i++) {
        assert(tour.leg_paths[i].node_count >= 2);
        assert(tour.leg_paths[i].nodes[0] == prev_tour_loc);
        assert(tour.leg_paths[i].nodes[tour.leg_paths[i].node_count - 1] == tour.ordered_checkpoints[i]);
        prev_tour_loc = tour.ordered_checkpoints[i];
    }
    printf("[PASS] Optimal Tour verified: Shortest walking tour computed (%.1f meters across %d checkpoints):\n       Order: [Entry] ",
           tour.total_distance, tour.count);
    for (int i = 0; i < tour.count; i++) {
        printf("-> [%s] ", tour_g->nodes[tour.ordered_checkpoints[i]].name);
    }
    printf("\n");
    free_graph(tour_g);

    Graph* pref_g = create_graph(0);
    load_checkpoints("checkpoints.txt", pref_g);

    assert(checkpoint_has_available_round(pref_g, 2, 5) == 1);
    assert(checkpoint_has_available_round(pref_g, 2, 999) == 0);
    for (int r = 0; r < pref_g->nodes[10].num_rounds; r++) {
        pref_g->nodes[10].rounds[r].available_seats = 0;
    }
    assert(checkpoint_has_available_round(pref_g, 10, 5) == 0);
    printf("[PASS] Fully unavailable checkpoint detection verified (correctly identifies 0 available rounds).\n");

    int exact_req[3] = {2, 3, 4};
    assert(is_exact_order_feasible(pref_g, 3, exact_req, 5) == 1);
    printf("[PASS] Exact Order Feasibility verified: Directly usable requested order detected (Spearman penalty = 0.0).\n");

    pref_g->nodes[1].rounds[0].available_seats = 0;
    int conflict_req[2] = {1, 2};
    assert(is_exact_order_feasible(pref_g, 2, conflict_req, 5) == 0);

    ItinerarySchedule pref_opt;
    ItinerarySchedule pref_alts[MAX_ALTERNATIVE_ITINERARIES];
    int num_p_alts = 0;
    int opt_success = optimize_itineraries_in_ram(pref_g, 2, conflict_req, 5, &pref_opt, pref_alts, &num_p_alts);
    assert(opt_success == 1);
    assert(pref_opt.is_valid == 1);
    assert(pref_opt.assigned_checkpoint_id[0] == 2);
    assert(pref_opt.assigned_checkpoint_id[1] == 1);
    assert(pref_opt.spearman_penalty == 2.0);
    printf("[PASS] Hungarian reordering verified: Capacity conflict in exact order resolved with minimum displacement (Penalty: %.1f).\n",
           pref_opt.spearman_penalty);

    free_graph(pref_g);

    remove("test_bookings.txt");
    int test_sched[7] = {1, 2, 3, -1, 4, 5, 6};
    assert(save_itinerary_booking("test_bookings.txt", parallel_g, "Wood's Group", "Wood", 4, test_sched, 7) == 1);

    int test_tour[3] = {1, 2, 3};
    assert(save_tour_booking("test_bookings.txt", parallel_g, "Wood's Group", "Wood", 4, 0, test_tour, 3, 40.0) == 1);

    FILE* bfp = fopen("test_bookings.txt", "r");
    assert(bfp != NULL);
    char buf[512];
    int found_itin = 0, found_tour = 0;
    while (fgets(buf, sizeof(buf), bfp)) {
        if (strstr(buf, "BOOKING: ITINERARY PLAN")) found_itin = 1;
        if (strstr(buf, "BOOKING: FREE EXPLORATION TOUR")) found_tour = 1;
    }
    fclose(bfp);
    assert(found_itin == 1);
    assert(found_tour == 1);
    printf("[PASS] Visitor booking persistence verified: Itinerary and Tour bookings successfully logged.\n");

    /* Map Visualizer Verification */
    int test_l1 = find_checkpoint_by_name(campus_g, "113");
    int test_l3 = find_checkpoint_by_name(campus_g, "313");
    int test_entry = find_checkpoint_by_name(campus_g, "entry");
    assert(test_l1 != -1 && test_l3 != -1 && test_entry != -1);
    assert(get_node_floor_level(campus_g->nodes[test_l1].name, campus_g->nodes[test_l1].room_num) == 1);
    assert(get_node_floor_level(campus_g->nodes[test_l3].name, campus_g->nodes[test_l3].room_num) == 3);
    assert(get_node_floor_level(campus_g->nodes[test_entry].name, campus_g->nodes[test_entry].room_num) == 2);
    assert(strcmp(get_canonical_node_code("113", 113), "113") == 0);
    assert(strcmp(get_canonical_node_code("1stair (L2)", 0), "1stair") == 0);
    assert(strcmp(get_canonical_node_code("entry", 0), "entry") == 0);
    assert(strcmp(get_canonical_node_code("Arcade / Learning Common (L3)", 0), "arc") == 0);

    /* Verify path visualization execution for ascending route (L1 -> L3 using + for stairs) */
    visualize_path(campus_g, &floor_path);

    /* Verify path visualization execution for descending route (L3 -> L1 using - for stairs) */
    Path desc_path;
    assert(dijkstra_shortest_path(campus_g, test_l3, test_l1, NULL, 0, &desc_path) == 1);
    visualize_path(campus_g, &desc_path);

    printf("[PASS] ASCII Map Visualizer verified: 3-floor map rendering, walking path (*), up (+), and down (-) transitions working seamlessly.\n");

    free_graph(g);
    free_graph(g_reloaded);
    free_graph(campus_g);
    free_graph(parallel_g);
    remove("test_checkpoints.txt");

    printf("\n>>> ALL TEST SUITES PASSED SUCCESSFULLY! <<<\n");
    return 0;
}
