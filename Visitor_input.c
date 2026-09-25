#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "graph.h"
#include "recommendation.h"
#include "storage.h"
#include "map_visualizer.h"

static void print_header() {
    printf("\n========================================================\n");
    printf("        KVIS OPEN HOUSE SMART VISITOR GUIDANCE         \n");
    printf("     Continuous Flow & Multi-Round Plan Optimization    \n");
    printf("========================================================\n");
}

static void display_round_checkpoints(Graph* g, int round_idx, const int* assigned_checkpoints, int total_rounds) {
    printf("\n--- Available Checkpoints for Round %d ---\n", round_idx + 1);
    printf(" ID | Checkpoint Name      | Room | Free Seats | Status\n");
    printf("----+----------------------+------+------------+----------------------------\n");

    for (int i = 0; i < g->num_checkpoints; i++) {
        if (strlen(g->nodes[i].name) == 0 || g->nodes[i].num_rounds == 0 || i == 0) continue;

        int avail = g->nodes[i].available_seats;
        int max_s = g->nodes[i].max_seats;
        if (round_idx >= 0 && round_idx < g->nodes[i].num_rounds) {
            avail = g->nodes[i].rounds[round_idx].available_seats;
            max_s = g->nodes[i].rounds[round_idx].max_seats;
        }

        int prev_round = -1;
        if (assigned_checkpoints) {
            for (int pr = 0; pr < total_rounds; pr++) {
                if (assigned_checkpoints[pr] == i) {
                    prev_round = pr;
                    break;
                }
            }
        }

        char status[64];
        if (prev_round != -1) {
            snprintf(status, sizeof(status), "Already Selected (Round %d)", prev_round + 1);
        } else if (avail <= 0) {
            snprintf(status, sizeof(status), "FULL");
        } else if (avail <= max_s * 0.25) {
            snprintf(status, sizeof(status), "Crowded");
        } else {
            snprintf(status, sizeof(status), "Available");
        }

        printf(" %2d | %-20s | %4d |   %2d/%2d    | %s\n",
               g->nodes[i].id,
               g->nodes[i].name,
               g->nodes[i].room_num,
               avail, max_s,
               status);
    }
    printf("----------------------------------------------------------------------------\n");
}

static void display_all_checkpoints_for_tour(Graph* g) {
    printf("\n--- Available Exhibition Checkpoints for Free Exploration ---\n");
    printf(" ID | Checkpoint Name      | Room | Floor   | Max Room Capacity\n");
    printf("----+----------------------+------+---------+-------------------\n");
    for (int i = 0; i < g->num_checkpoints; i++) {
        if (strlen(g->nodes[i].name) == 0 || g->nodes[i].num_rounds == 0 || i == 0) continue;

        int floor_num = g->nodes[i].room_num / 100;
        char floor_str[16];
        if (floor_num > 0) {
            snprintf(floor_str, sizeof(floor_str), "Level %d", floor_num);
        } else {
            snprintf(floor_str, sizeof(floor_str), "Auditorium");
        }

        printf(" %2d | %-20s | %4d | %-7s | %3d visitors\n",
               g->nodes[i].id,
               g->nodes[i].name,
               g->nodes[i].room_num,
               floor_str,
               g->nodes[i].max_seats);
    }
    printf("------------------------------------------------------------\n");
    printf("Note: Free exploration mode does not restrict you by available seats.\n");
    printf("      The algorithm will calculate the mathematically optimal route!\n");
}

static void display_capacity_overview(Graph* g, int group_size) {
    int ref_node = -1;
    for (int i = 0; i < g->num_checkpoints; i++) {
        if (g->nodes[i].num_rounds > 0) { ref_node = i; break; }
    }
    if (ref_node == -1) return;

    printf("\n--- Available Exhibition Checkpoints Overview ---\n");
    printf(" ID | Checkpoint Name      | Room | Floor   | Max | Feasible for %d visitors?\n", group_size);
    printf("----+----------------------+------+---------+-----+---------------------------\n");
    for (int i = 0; i < g->num_checkpoints; i++) {
        if (strlen(g->nodes[i].name) == 0 || g->nodes[i].num_rounds == 0 || i == 0) continue;
        int fnum = g->nodes[i].room_num / 100;
        char fstr[16];
        if (fnum > 0) snprintf(fstr, sizeof(fstr), "Level %d", fnum);
        else snprintf(fstr, sizeof(fstr), "Audi");

        int has_avail = checkpoint_has_available_round(g, i, group_size);
        printf(" %2d | %-20s | %4d | %-7s | %3d | %s\n",
               g->nodes[i].id,
               g->nodes[i].name,
               g->nodes[i].room_num,
               fstr,
               g->nodes[i].max_seats,
               has_avail ? "YES (Has Open Rounds)" : "NO (All Rounds Full)");
    }
    printf("--------------------------------------------------------------------------------\n");
}

static void print_confirmed_summary(Graph* g, const char* group_name, const char* member_name, int group_size, int total_rounds, int ref_node, const int* assigned_checkpoints) {
    printf("\n\n========================================================================\n");
    printf("               CONFIRMED ITINERARY SUMMARY PLAN FOR GROUP               \n");
    printf("========================================================================\n");
    printf(" Group Name     : %s\n", group_name);
    printf(" Representative : %s\n", member_name);
    printf(" Group Size     : %d visitor%s\n", group_size, (group_size > 1 ? "s" : ""));
    printf(" Booking Status : CONFIRMED & LOGGED TO visitor_bookings.txt\n");
    printf("------------------------------------------------------------------------\n");

    for (int r = 0; r < total_rounds; r++) {
        Round rnd = g->nodes[ref_node].rounds[r];
        int sh = (int)rnd.start_time, sm = (int)((rnd.start_time - sh) * 60);
        int eh = (int)rnd.end_time,   em = (int)((rnd.end_time - eh) * 60);

        if (rnd.is_lunch_break) {
            printf("  [Round %d]  %02d:%02d - %02d:%02d  |  *** LUNCH BREAK (Empty Section - Relax & Eat) ***\n",
                   r + 1, sh, sm, eh, em);
        } else {
            int cp_id = assigned_checkpoints[r];
            if (cp_id > 0 && cp_id < g->num_checkpoints) {
                printf("  [Round %d]  %02d:%02d - %02d:%02d  |  %s (Room %d) - [CONFIRMED]\n",
                       r + 1, sh, sm, eh, em,
                       g->nodes[cp_id].name, g->nodes[cp_id].room_num);
            } else {
                printf("  [Round %d]  %02d:%02d - %02d:%02d  |  (Free Time / Open Campus Walk)\n",
                       r + 1, sh, sm, eh, em);
            }
        }
    }
    printf("========================================================================\n");

    printf("\nDo you want walking route guidance between your scheduled checkpoints?\n");
    printf(" [1] Yes (Show step-by-step route)\n");
    printf(" [2] No\n");
    printf("Select: ");
    int want_guidance = 1;
    if (scanf("%d", &want_guidance) == 1 && want_guidance == 1) {
        int prev_loc = 0;
        printf("\n--- WALKING ITINERARY GUIDANCE ---\n");
        for (int r = 0; r < total_rounds; r++) {
            int cp_id = assigned_checkpoints[r];
            if (cp_id > 0 && cp_id < g->num_checkpoints) {
                Path nav;
                if (dijkstra_shortest_path(g, prev_loc, cp_id, NULL, 0, &nav)) {
                    printf("\nLeg -> Round %d: From [%s] to [%s] (Total: %.1f m)\n",
                           r + 1, g->nodes[prev_loc].name, g->nodes[cp_id].name, nav.total_distance);
                    printf("Path: ");
                    for (int n = 0; n < nav.node_count; n++) {
                        printf("[%s]%s", g->nodes[nav.nodes[n]].name, (n < nav.node_count - 1 ? " ──> " : ""));
                    }
                    printf("\n");
                    visualize_path(g, &nav);
                }
                prev_loc = cp_id;
            }
        }
        printf("----------------------------------\n");
    }

    printf("\nThank you, %s! Have a wonderful day visiting KVIS Open House!\n\n", group_name);
}

static void handle_preference_based_itinerary(Graph* g, const char* member_name, const char* group_name, int group_size, int total_rounds, int ref_node) {
    int act_rounds[MAX_ITINERARY_SLOTS];
    int num_slots = get_activity_rounds(g, act_rounds, MAX_ITINERARY_SLOTS);
    if (num_slots <= 0) {
        printf("[Error] No activity rounds available.\n");
        return;
    }

    display_capacity_overview(g, group_size);

    int k = 0;
    while (1) {
        printf("\nHow many checkpoints would you like to visit today? (1 to %d): ", num_slots);
        if (scanf("%d", &k) == 1 && k >= 1 && k <= num_slots) {
            break;
        }
        printf("[Error] Please enter a number between 1 and %d.\n", num_slots);
        int c; while ((c = getchar()) != '\n' && c != EOF);
    }

    int requested[MAX_ITINERARY_SLOTS];
    for (int i = 0; i < k; i++) {
        while (1) {
            printf("Enter Checkpoint ID for Choice #%d (in your preferred visiting order): ", i + 1);
            int cid = -1;
            if (scanf("%d", &cid) != 1) {
                int c; while ((c = getchar()) != '\n' && c != EOF);
                continue;
            }
            if (cid <= 0 || cid >= g->num_checkpoints || strlen(g->nodes[cid].name) == 0 || g->nodes[cid].num_rounds == 0) {
                printf("[Error] Invalid checkpoint ID. Please choose an exhibition room from the table above.\n");
                continue;
            }
            int is_dup = 0;
            for (int p = 0; p < i; p++) {
                if (requested[p] == cid) { is_dup = 1; break; }
            }
            if (is_dup) {
                printf("[Error] You already chose %s (Room %d)! Each checkpoint can only be chosen once.\n",
                       g->nodes[cid].name, g->nodes[cid].room_num);
                continue;
            }
            requested[i] = cid;
            break;
        }
    }

    for (int i = 0; i < k; i++) {
        if (requested[i] == -1) continue;
        while (requested[i] != -1 && !checkpoint_has_available_round(g, requested[i], group_size)) {
            printf("\n========================================================================\n");
            printf(" [NO AVAILABLE ROUNDS] %s (Room %d)\n",
                   g->nodes[requested[i]].name, g->nodes[requested[i]].room_num);
            printf(" This checkpoint has insufficient seats across ALL rounds today for %d visitors!\n", group_size);
            printf("========================================================================\n");
            printf("Options for Choice #%d:\n", i + 1);
            printf(" [1] Choose another checkpoint to replace %s\n", g->nodes[requested[i]].name);
            printf(" [2] Keep this slot as Free Time / Open Campus Walk\n");
            printf("Select an option (1-2): ");
            int opt = 1;
            if (scanf("%d", &opt) != 1) opt = 1;

            if (opt == 1) {
                printf("\nAvailable checkpoints with open seats today:\n");
                for (int c = 1; c < g->num_checkpoints; c++) {
                    if (strlen(g->nodes[c].name) == 0 || g->nodes[c].num_rounds == 0) continue;
                    int in_use = 0;
                    for (int p = 0; p < k; p++) {
                        if (requested[p] == c) { in_use = 1; break; }
                    }
                    if (!in_use && checkpoint_has_available_round(g, c, group_size)) {
                        printf("  [%d] %-20s (Room %d)\n", c, g->nodes[c].name, g->nodes[c].room_num);
                    }
                }
                printf("Enter replacement Checkpoint ID: ");
                int new_cid = -1;
                if (scanf("%d", &new_cid) == 1 && new_cid > 0 && new_cid < g->num_checkpoints &&
                    strlen(g->nodes[new_cid].name) > 0 && g->nodes[new_cid].num_rounds > 0) {
                    int is_dup = 0;
                    for (int p = 0; p < k; p++) {
                        if (p != i && requested[p] == new_cid) { is_dup = 1; break; }
                    }
                    if (!is_dup) {
                        requested[i] = new_cid;
                        printf("--> Replaced with: %s (Room %d)\n", g->nodes[new_cid].name, g->nodes[new_cid].room_num);
                    } else {
                        printf("[Error] Checkpoint already selected elsewhere.\n");
                    }
                }
            } else {
                printf("--> Choice #%d set to Free Time.\n", i + 1);
                requested[i] = -1;
                break;
            }
        }
    }

    int active_req[MAX_ITINERARY_SLOTS];
    int active_count = 0;
    for (int i = 0; i < k; i++) {
        if (requested[i] > 0) {
            active_req[active_count++] = requested[i];
        }
    }

    ItinerarySchedule final_sched;
    for (int s = 0; s < MAX_ITINERARY_SLOTS; s++) {
        final_sched.assigned_checkpoint_id[s] = -1;
        final_sched.slot_round_idx[s] = (s < num_slots) ? act_rounds[s] : -1;
    }
    final_sched.spearman_penalty = 0.0;
    final_sched.is_valid = 1;

    if (active_count == 0) {
        printf("\nAll activity slots scheduled as Free Time.\n");
    } else if (is_exact_order_feasible(g, active_count, active_req, group_size)) {

        printf("\n========================================================================\n");
        printf("                     EXACT ORDER FEASIBLE!                              \n");
        printf("========================================================================\n");
        printf(" Great news! All your chosen checkpoints have open capacity in your exact\n");
        printf(" preferred order! No schedule reordering needed (Spearman Penalty: 0.0).\n");
        printf("========================================================================\n");
        for (int i = 0; i < active_count; i++) {
            final_sched.assigned_checkpoint_id[i] = active_req[i];
        }
    } else {

        printf("\n========================================================================\n");
        printf("               CAPACITY CONFLICT IN EXACT ORDER                         \n");
        printf("========================================================================\n");
        printf(" Your requested order cannot be used directly due to room capacity limits\n");
        printf(" in certain rounds. to find the optimal schedule with minimal displacement\n");
        printf(" from your requested order...\n");
        printf("========================================================================\n");

        ItinerarySchedule alts[MAX_ALTERNATIVE_ITINERARIES];
        int num_alts = 0;
        int success = optimize_itineraries_in_ram(g, active_count, active_req, group_size, &final_sched, alts, &num_alts);

        if (success && final_sched.is_valid) {
            printf("\n>>> HUNGARIAN ALGORITHM OPTIMIZATION COMPLETE <<<\n");
            printf(" The optimal schedule matching your preferences as closely as possible\n");
            printf(" (Schedule Displacement Penalty: %.1f).\n",
                   final_sched.spearman_penalty);
            printf("------------------------------------------------------------------------\n");
            for (int s = 0; s < num_slots; s++) {
                int cid = final_sched.assigned_checkpoint_id[s];
                int r_idx = act_rounds[s];
                Round rnd = g->nodes[ref_node].rounds[r_idx];
                int sh = (int)rnd.start_time, sm = (int)((rnd.start_time - sh) * 60);
                int eh = (int)rnd.end_time,   em = (int)((rnd.end_time - eh) * 60);
                if (cid > 0) {
                    printf("  Slot %d (Round %d, %02d:%02d - %02d:%02d): %-20s (Room %4d) [Seats Left: %d]\n",
                           s + 1, r_idx + 1, sh, sm, eh, em,
                           g->nodes[cid].name, g->nodes[cid].room_num,
                           g->nodes[cid].rounds[r_idx].available_seats);
                } else {
                    printf("  Slot %d (Round %d, %02d:%02d - %02d:%02d): (Free Time)\n",
                           s + 1, r_idx + 1, sh, sm, eh, em);
                }
            }
        } else {
            printf("\n[Notice] Some chosen checkpoints have conflicting single openings.\n");
            printf("Applying best available assignment...\n");
        }
    }

    int assigned_checkpoints[MAX_ROUNDS];
    for (int r = 0; r < total_rounds; r++) assigned_checkpoints[r] = -1;

    for (int s = 0; s < num_slots; s++) {
        int cid = final_sched.assigned_checkpoint_id[s];
        int r_idx = act_rounds[s];
        if (cid > 0) {
            assigned_checkpoints[r_idx] = cid;
            g->nodes[cid].rounds[r_idx].available_seats -= group_size;
            if (g->nodes[cid].rounds[r_idx].available_seats < 0) {
                g->nodes[cid].rounds[r_idx].available_seats = 0;
            }
        }
    }
    save_checkpoints("checkpoints.txt", g);
    save_itinerary_booking("visitor_bookings.txt", g, group_name, member_name, group_size, assigned_checkpoints, total_rounds);

    print_confirmed_summary(g, group_name, member_name, group_size, total_rounds, ref_node, assigned_checkpoints);
}

static void handle_step_by_step_itinerary(Graph* g, const char* member_name, const char* group_name, int group_size, int total_rounds, int ref_node) {
    int assigned_checkpoints[MAX_ROUNDS];
    for (int r = 0; r < MAX_ROUNDS; r++) {
        assigned_checkpoints[r] = -1;
    }
    int current_loc = 0;

    for (int r = 0; r < total_rounds; r++) {
        Round rnd = g->nodes[ref_node].rounds[r];
        int sh = (int)rnd.start_time, sm = (int)((rnd.start_time - sh) * 60);
        int eh = (int)rnd.end_time,   em = (int)((rnd.end_time - eh) * 60);

        if (rnd.is_lunch_break) {
            printf("\n========================================================================\n");
            printf(" [Round %d] %02d:%02d - %02d:%02d : *** LUNCH BREAK (Empty Section) ***\n",
                   r + 1, sh, sm, eh, em);
            printf(" All exhibitions are closed for lunch. Relax, refuel, and enjoy your meal!\n");
            printf("========================================================================\n");
            continue;
        }

        while (1) {
            display_round_checkpoints(g, r, assigned_checkpoints, total_rounds);
            printf("\n[Round %d] %02d:%02d - %02d:%02d\n", r + 1, sh, sm, eh, em);
            printf("Enter Checkpoint ID to visit (or -1 for free time, -2 to finish schedule): ");
            int chosen_id = -1;
            if (scanf("%d", &chosen_id) != 1) {
                chosen_id = -1;
            }

            if (chosen_id == -2) {
                printf("\nFinishing checkpoint selection early. Proceeding to final plan...\n");
                r = total_rounds;
                break;
            }

            if (chosen_id == -1) {
                assigned_checkpoints[r] = -1;
                printf("--> Round %d set to: (Free Time / Open Campus Walk)\n", r + 1);
                break;
            }

            if (chosen_id < 0 || chosen_id >= g->num_checkpoints ||
                g->nodes[chosen_id].num_rounds <= 0 || chosen_id == 0) {
                printf("[Error] Invalid checkpoint ID. Please select a valid numbered exhibition checkpoint.\n");
                continue;
            }

            int already_chosen_round = -1;
            for (int pr = 0; pr < total_rounds; pr++) {
                if (assigned_checkpoints[pr] == chosen_id) {
                    already_chosen_round = pr;
                    break;
                }
            }
            if (already_chosen_round != -1) {
                printf("\n[ERROR] You have already selected %s (Room %d) for Round %d!\n",
                       g->nodes[chosen_id].name, g->nodes[chosen_id].room_num, already_chosen_round + 1);
                printf("Rule: Each checkpoint can only be visited once. Please choose a different checkpoint.\n");
                continue;
            }

            int avail = g->nodes[chosen_id].rounds[r].available_seats;

            if (avail >= group_size) {

                g->nodes[chosen_id].rounds[r].available_seats -= group_size;
                if (g->nodes[chosen_id].rounds[r].available_seats < 0) {
                    g->nodes[chosen_id].rounds[r].available_seats = 0;
                }
                save_checkpoints("checkpoints.txt", g);

                assigned_checkpoints[r] = chosen_id;
                current_loc = chosen_id;
                printf("\n[CONFIRMED] %s scheduled for %s (Room %d) in Round %d.\n",
                       group_name, g->nodes[chosen_id].name, g->nodes[chosen_id].room_num, r + 1);
                printf("           (Free seats in Round %d immediately decreased to: %d)\n",
                       r + 1, g->nodes[chosen_id].rounds[r].available_seats);
                break;
            } else {

                printf("\n========================================================\n");
                printf(" [ROOM FULL] %s (Room %d) has only %d seats left (need %d) for Round %d!\n",
                       g->nodes[chosen_id].name, g->nodes[chosen_id].room_num, avail, group_size, r + 1);
                printf("========================================================\n");

                int excluded[MAX_ROUNDS];
                int num_excluded = 0;
                for (int pr = 0; pr < total_rounds; pr++) {
                    if (assigned_checkpoints[pr] > 0) {
                        excluded[num_excluded++] = assigned_checkpoints[pr];
                    }
                }

                int alt_id = hungarian_suggest_alternative(g, current_loc, r, chosen_id, group_size, excluded, num_excluded);
                if (alt_id != -1) {
                    printf("\n>>> HUNGARIAN ALGORITHM RECOMMENDATION <<<\n");
                    printf("Based on minimal walking displacement and open capacity in Round %d:\n", r + 1);
                    printf(" • Suggested Alternative : %s (ID: %d, Room %d)\n",
                           g->nodes[alt_id].name, alt_id, g->nodes[alt_id].room_num);
                    printf(" • Available Seats       : %d / %d\n",
                           g->nodes[alt_id].rounds[r].available_seats,
                           g->nodes[alt_id].rounds[r].max_seats);
                    printf("--------------------------------------------------------\n");
                    printf("Options:\n");
                    printf(" [1] Accept Hungarian Suggestion (%s)\n", g->nodes[alt_id].name);
                    printf(" [2] No, choose a different checkpoint myself\n");
                    printf("Select: ");

                    int resp = 1;
                    if (scanf("%d", &resp) == 1 && resp == 1) {
                        g->nodes[alt_id].rounds[r].available_seats -= group_size;
                        if (g->nodes[alt_id].rounds[r].available_seats < 0) {
                            g->nodes[alt_id].rounds[r].available_seats = 0;
                        }
                        save_checkpoints("checkpoints.txt", g);

                        assigned_checkpoints[r] = alt_id;
                        current_loc = alt_id;
                        printf("\n[CONFIRMED] %s scheduled for %s (Room %d) in Round %d.\n",
                               group_name, g->nodes[alt_id].name, g->nodes[alt_id].room_num, r + 1);
                        printf("           (Free seats in Round %d immediately decreased to: %d)\n",
                               r + 1, g->nodes[alt_id].rounds[r].available_seats);
                        break;
                    } else {
                        printf("\nNo problem! Please choose another checkpoint for Round %d:\n", r + 1);
                        continue;
                    }
                } else {
                    printf("\n[Notice] No alternative unvisited checkpoints have sufficient capacity in Round %d.\n", r + 1);
                    printf("Please select a different checkpoint or enter -1 for free time.\n");
                    continue;
                }
            }
        }
    }

    save_checkpoints("checkpoints.txt", g);
    save_itinerary_booking("visitor_bookings.txt", g, group_name, member_name, group_size, assigned_checkpoints, total_rounds);
    print_confirmed_summary(g, group_name, member_name, group_size, total_rounds, ref_node, assigned_checkpoints);
}

static void handle_capacity_aware_itinerary(Graph* g) {
    int ref_node = -1;
    for (int i = 0; i < g->num_checkpoints; i++) {
        if (g->nodes[i].num_rounds > 0) { ref_node = i; break; }
    }
    if (ref_node == -1) {
        printf("[Error] No round schedules found in graph.\n");
        return;
    }
    int total_rounds = g->nodes[ref_node].num_rounds;

    char member_name[64] = "Visitor";
    char group_name[96] = "Visitor's Group";
    int group_size = 1;

    printf("\n--- Group Registration ---\n");
    printf("Enter the name of a member in your group: ");
    if (scanf("%63s", member_name) == 1 && strlen(member_name) > 0) {
        snprintf(group_name, sizeof(group_name), "%s's Group", member_name);
    }

    printf("Enter total group size (number of visitors): ");
    if (scanf("%d", &group_size) != 1 || group_size <= 0) {
        group_size = 1;
    }

    printf("\n========================================================\n");
    printf(" Welcome, %s (%d visitor%s)!\n", group_name, group_size, (group_size > 1 ? "s" : ""));
    printf("========================================================\n");

    printf("\n--- Daily Parallel Schedule Overview ---\n");
    for (int r = 0; r < total_rounds; r++) {
        Round rnd = g->nodes[ref_node].rounds[r];
        int sh = (int)rnd.start_time, sm = (int)((rnd.start_time - sh) * 60);
        int eh = (int)rnd.end_time,   em = (int)((rnd.end_time - eh) * 60);
        if (rnd.is_lunch_break) {
            printf(" Round %d (%02d:%02d - %02d:%02d) : *** LUNCH BREAK (Empty Section) ***\n",
                   r + 1, sh, sm, eh, em);
        } else {
            printf(" Round %d (%02d:%02d - %02d:%02d) : Activity Round\n",
                   r + 1, sh, sm, eh, em);
        }
    }
    printf("----------------------------------------\n");

    printf("\nSelect Planning Mode:\n");
    printf(" [1] Smart Preference Optimizer (Pick checkpoints in preferred order -> system checks & optimizes)\n");
    printf(" [2] Manual Step-by-Step Round Selection (Select round-by-round)\n");
    printf("Select mode (1-2, default 1): ");
    int plan_mode = 1;
    if (scanf("%d", &plan_mode) != 1) {
        plan_mode = 1;
    }

    if (plan_mode == 2) {
        handle_step_by_step_itinerary(g, member_name, group_name, group_size, total_rounds, ref_node);
    } else {
        handle_preference_based_itinerary(g, member_name, group_name, group_size, total_rounds, ref_node);
    }
}

static void handle_free_exploration_tour(Graph* g) {
    char member_name[64] = "Visitor";
    char group_name[96] = "Visitor's Group";
    int group_size = 1;

    printf("\n========================================================\n");
    printf("          FREE EXPLORATION TOUR OPTIMIZER               \n");
    printf("    (Select Desired Checkpoints - Shortest Route Tour)   \n");
    printf("========================================================\n");

    printf("Enter the name of a member in your group: ");
    if (scanf("%63s", member_name) == 1 && strlen(member_name) > 0) {
        snprintf(group_name, sizeof(group_name), "%s's Group", member_name);
    }

    printf("Enter total group size (number of visitors): ");
    if (scanf("%d", &group_size) != 1 || group_size <= 0) {
        group_size = 1;
    }

    display_all_checkpoints_for_tour(g);

    int start_node = 0;
    printf("\nEnter starting location (Checkpoint ID, default 0 for Main Entry): ");
    if (scanf("%d", &start_node) != 1 || start_node < 0 || start_node >= g->num_checkpoints) {
        start_node = 0;
    }
    printf("--> Starting Tour from: [%s] (Room %d)\n",
           g->nodes[start_node].name, g->nodes[start_node].room_num);

    int available_count = 0;
    for (int i = 0; i < g->num_checkpoints; i++) {
        if (i != 0 && g->nodes[i].num_rounds > 0 && strlen(g->nodes[i].name) > 0) {
            available_count++;
        }
    }
    if (available_count > 10) available_count = 10;

    int num_targets = 0;
    while (1) {
        printf("\nHow many checkpoints would you like to visit? (1 to %d): ", available_count);
        if (scanf("%d", &num_targets) == 1 && num_targets >= 1 && num_targets <= available_count) {
            break;
        }
        printf("[Error] Please enter a valid number between 1 and %d.\n", available_count);
        int c;
        while ((c = getchar()) != '\n' && c != EOF);
    }

    int targets[MAX_CHECKPOINTS];
    for (int i = 0; i < num_targets; i++) {
        while (1) {
            printf("Enter Checkpoint ID for Stop #%d: ", i + 1);
            int cp_id = -1;
            if (scanf("%d", &cp_id) != 1) {
                int c;
                while ((c = getchar()) != '\n' && c != EOF);
                printf("[Error] Please enter a valid integer ID.\n");
                continue;
            }

            if (cp_id < 0 || cp_id >= g->num_checkpoints || strlen(g->nodes[cp_id].name) == 0 || cp_id == 0) {
                printf("[Error] Checkpoint ID %d is invalid or not an exhibition room. Please choose from the table above.\n", cp_id);
                continue;
            }

            int is_dup = 0;
            for (int prev = 0; prev < i; prev++) {
                if (targets[prev] == cp_id) {
                    is_dup = 1;
                    break;
                }
            }
            if (is_dup) {
                printf("[Error] Checkpoint %s (Room %d) is already selected! Please choose a different checkpoint.\n",
                       g->nodes[cp_id].name, g->nodes[cp_id].room_num);
                continue;
            }

            targets[i] = cp_id;
            break;
        }
    }

    printf("\nCalculating mathematically optimal shortest walking route for %s...\n", group_name);
    OptimalTour tour = find_optimal_tour(g, start_node, targets, num_targets);

    double entered_distance = 0.0;
    int prev_entered = start_node;
    for (int i = 0; i < num_targets; i++) {
        Path p;
        if (dijkstra_shortest_path(g, prev_entered, targets[i], NULL, 0, &p)) {
            entered_distance += p.total_distance;
        }
        prev_entered = targets[i];
    }

    printf("\n========================================================================\n");
    printf("         OPTIMAL EXPLORATION TOUR FOR %s         \n", group_name);
    printf("========================================================================\n");
    printf(" Group Representative : %s\n", member_name);
    printf(" Group Size           : %d visitor%s\n", group_size, (group_size > 1 ? "s" : ""));
    printf(" Starting Location    : [%s] (Room %d)\n", g->nodes[start_node].name, g->nodes[start_node].room_num);
    printf(" Checkpoints Visited  : %d exhibitions\n", tour.count);
    printf(" Optimal Walk Distance: %.1f meters\n", tour.total_distance);
    if (entered_distance > tour.total_distance + 0.1) {
        printf(" Efficiency Benefit   : ✨ Saved %.1f meters of walking! (%.1f m vs %.1f m)\n",
               entered_distance - tour.total_distance, entered_distance, tour.total_distance);
    }
    printf("------------------------------------------------------------------------\n");

    printf("\n>>> OPTIMAL VISITING SEQUENCE <<<\n");
    printf("  [%s]", g->nodes[start_node].name);
    for (int i = 0; i < tour.count; i++) {
        int cid = tour.ordered_checkpoints[i];
        printf(" ──> [%s] (Room %d)", g->nodes[cid].name, g->nodes[cid].room_num);
    }
    printf("\n");

    printf("\n--- STEP-BY-STEP TURN-BY-TURN WALKING DIRECTIONS ---\n");
    int prev_loc = start_node;
    for (int i = 0; i < tour.count; i++) {
        int cid = tour.ordered_checkpoints[i];
        printf("\nLeg %d: [%s] ──> [%s] (%.1f meters)\n",
               i + 1, g->nodes[prev_loc].name, g->nodes[cid].name, tour.leg_paths[i].total_distance);
        printf("  Corridor Navigation: ");
        for (int n = 0; n < tour.leg_paths[i].node_count; n++) {
            printf("[%s]%s", g->nodes[tour.leg_paths[i].nodes[n]].name,
                   (n < tour.leg_paths[i].node_count - 1 ? " ──> " : ""));
        }
        printf("\n");
        visualize_path(g, &tour.leg_paths[i]);
        prev_loc = cid;
    }
    printf("----------------------------------------------------\n");

    printf("\n========================================================================\n");
    printf("              CONFIRMED TOUR SUMMARY: %s              \n", group_name);
    printf("========================================================================\n");
    printf(" Stop # | Checkpoint Name      | Room | Floor   | Leg Distance\n");
    printf("--------+----------------------+------+---------+-------------\n");
    for (int i = 0; i < tour.count; i++) {
        int cid = tour.ordered_checkpoints[i];
        int fnum = g->nodes[cid].room_num / 100;
        char fstr[16];
        if (fnum > 0) snprintf(fstr, sizeof(fstr), "Level %d", fnum);
        else snprintf(fstr, sizeof(fstr), "Auditorium");

        printf("   %2d   | %-20s | %4d | %-7s |  %5.1f m\n",
               i + 1,
               g->nodes[cid].name,
               g->nodes[cid].room_num,
               fstr,
               tour.leg_paths[i].total_distance);
    }
    printf("========================================================================\n");
    printf(" TOTAL ESTIMATED WALKING DISTANCE: %.1f meters\n", tour.total_distance);
    printf(" Booking Status                  : CONFIRMED & LOGGED TO visitor_bookings.txt\n");
    printf("========================================================================\n");

    save_tour_booking("visitor_bookings.txt", g, group_name, member_name, group_size, start_node, tour.ordered_checkpoints, tour.count, tour.total_distance);

    printf("\nThank you, %s! Enjoy your personalized exploration of KVIS Open House!\n", group_name);
}

static void handle_map_visualizer_menu(Graph* g) {
    while (1) {
        printf("\n========================================================\n");
        printf("         ROUTE PATH VISUALIZER & CAMPUS MAP             \n");
        printf("========================================================\n");
        printf(" [1] View Level 1 Floor Map (Ground / Level 1)\n");
        printf(" [2] View Level 2 Floor Map (Main Entry / Level 2)\n");
        printf(" [3] View Level 3 Floor Map (Arc / Level 3)\n");
        printf(" [4] View All Floor Maps (Levels 1, 2, and 3)\n");
        printf(" [5] Find Shortest Path & Visualize Route (Any 2 Locations)\n");
        printf(" [6] Return to Visitor Menu\n");
        printf("Select an option (1-6): ");

        int opt = 0;
        if (scanf("%d", &opt) != 1) {
            int c; while ((c = getchar()) != '\n' && c != EOF);
            break;
        }

        if (opt == 1) {
            display_floor_map(1);
        } else if (opt == 2) {
            display_floor_map(2);
        } else if (opt == 3) {
            display_floor_map(3);
        } else if (opt == 4) {
            display_all_floor_maps();
        } else if (opt == 5) {
            printf("\n--- Available Exhibition Checkpoints & Locations ---\n");
            for (int i = 0; i < g->num_checkpoints; i++) {
                if (strlen(g->nodes[i].name) == 0) continue;
                int f = get_node_floor_level(g->nodes[i].name, g->nodes[i].room_num);
                printf("  [%2d] %-20s (Room %4d, Level %d)\n", i, g->nodes[i].name, g->nodes[i].room_num, f);
            }
            int u = 0, v = 1;
            printf("\nEnter starting location ID (default 0 for Entry): ");
            if (scanf("%d", &u) != 1 || u < 0 || u >= g->num_checkpoints) u = 0;
            printf("Enter destination location ID: ");
            if (scanf("%d", &v) != 1 || v < 0 || v >= g->num_checkpoints) {
                printf("[Error] Invalid destination location ID.\n");
                continue;
            }

            Path p;
            if (dijkstra_shortest_path(g, u, v, NULL, 0, &p)) {
                visualize_path(g, &p);
            } else {
                printf("[Notice] No walking route found between [%s] and [%s].\n",
                       g->nodes[u].name, g->nodes[v].name);
            }
        } else if (opt == 6) {
            break;
        } else {
            printf("[Error] Invalid choice. Please enter 1-6.\n");
        }
    }
}

int main() {
    Graph* g = create_graph(0);

    int cp_count = load_checkpoints("checkpoints.txt", g);
    int edge_count = load_distances("distance.txt", g);

    print_header();
    printf("Loaded %d checkpoints and %d corridor connections.\n", cp_count, edge_count);

    if (cp_count <= 0) {
        printf("\n[Warning] No checkpoints found in 'checkpoints.txt'.\n");
        printf("Please run the Staff module first to set up checkpoints, or load test data.\n");
        free_graph(g);
        return 1;
    }

    while (1) {
        printf("\n========================================================\n");
        printf("                     VISITOR MENU                       \n");
        printf("========================================================\n");
        printf(" [1] Smart Capacity-Aware Itinerary (Continuous Selection & Rounds)\n");
        printf(" [2] Free Exploration Tour (Pick Checkpoints Freely & Find Optimal Route)\n");
        printf(" [3] Route Path Visualizer & Campus Map (View Map & Visualize Path to Any Room)\n");
        printf(" [4] Exit\n");
        printf("Select an option (1-4): ");

        int choice = 0;
        if (scanf("%d", &choice) != 1) {
            break;
        }

        if (choice == 1) {
            handle_capacity_aware_itinerary(g);
        } else if (choice == 2) {
            handle_free_exploration_tour(g);
        } else if (choice == 3) {
            handle_map_visualizer_menu(g);
        } else if (choice == 4) {
            printf("\nExiting Visitor Guidance System. Goodbye!\n");
            break;
        } else {
            printf("[Error] Invalid choice. Please enter 1, 2, 3, or 4.\n");
        }
    }

    free_graph(g);
    return 0;
}
