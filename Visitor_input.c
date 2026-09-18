#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "graph.h"
#include "recommendation.h"
#include "storage.h"

static void print_header() {
    printf("\n========================================================\n");
    printf("        KVIS OPEN HOUSE SMART VISITOR GUIDANCE         \n");
    printf("   \"Optimizing visitor flow & eliminating crowding\"    \n");
    printf("========================================================\n");
}

static void display_available_checkpoints(Graph* g, int round_idx) {
    printf("\n--- Available Checkpoints for Round %d ---\n", round_idx + 1);
    printf(" ID | Name                 | Room | Free Seats | Status\n");
    printf("----+----------------------+------+------------+---------------\n");

    for (int i = 0; i < g->num_checkpoints; i++) {
        if (strlen(g->nodes[i].name) == 0 || g->nodes[i].num_rounds == 0) continue;
        
        int avail = g->nodes[i].available_seats;
        int max_s = g->nodes[i].max_seats;
        if (round_idx >= 0 && round_idx < g->nodes[i].num_rounds) {
            avail = g->nodes[i].rounds[round_idx].available_seats;
            max_s = g->nodes[i].rounds[round_idx].max_seats;
        }

        const char* status = "Available";
        if (avail <= 0) status = "FULL";
        else if (avail <= max_s * 0.25) status = "Crowded";

        printf(" %2d | %-20s | %4d |   %2d/%2d    | %s\n",
               g->nodes[i].id,
               g->nodes[i].name,
               g->nodes[i].room_num,
               avail, max_s,
               status);
    }
    printf("-------------------------------------------------------\n");
}

int main() {
    Graph* g = create_graph(0);

    // 1. Load Data
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
        printf("\n--------------------------------------------------------\n");
        printf("                     VISITOR MENU                       \n");
        printf("--------------------------------------------------------\n");
        printf(" [1] Find Best Next Checkpoint & Get Navigation Route\n");
        printf(" [2] Exit\n");
        printf("Select option: ");

        int main_choice = 0;
        if (scanf("%d", &main_choice) != 1 || main_choice == 2) {
            printf("\nThank you for visiting KVIS Open House! Have a great day.\n");
            break;
        }

        if (main_choice != 1) {
            printf("Invalid selection.\n");
            continue;
        }

        // --- Step 1: Group / Individual Mode ---
        int mode_choice = 1;
        int group_size = 1;
        printf("\nHow are you visiting today?\n");
        printf(" [1] Individual (1 person)\n");
        printf(" [2] Group (Multiple visitors)\n");
        printf("Select: ");
        if (scanf("%d", &mode_choice) == 1 && mode_choice == 2) {
            printf("Enter total group size: ");
            if (scanf("%d", &group_size) != 1 || group_size <= 0) {
                group_size = 1;
            }
        } else {
            group_size = 1;
        }

        // --- Step 2: Choose Round ---
        int round_choice = 1;
        printf("\nSelect activity round:\n");
        for (int r = 0; r < 3; r++) {
            printf(" [%d] Round %d\n", r + 1, r + 1);
        }
        printf("Select round (1-3): ");
        if (scanf("%d", &round_choice) != 1 || round_choice < 1 || round_choice > MAX_ROUNDS) {
            round_choice = 1;
        }
        int round_idx = round_choice - 1;

        // --- Step 3: Current Location ---
        printf("\nWhere are you currently located?\n");
        for (int i = 0; i < g->num_checkpoints; i++) {
            if (strlen(g->nodes[i].name) > 0 && (g->nodes[i].num_rounds > 0 || i == 0)) {
                printf(" [%d] %s (Room %d)\n", g->nodes[i].id, g->nodes[i].name, g->nodes[i].room_num);
            }
        }
        printf("Enter your current checkpoint ID: ");
        int current_cp = 0;
        if (scanf("%d", &current_cp) != 1 || current_cp < 0 || current_cp >= g->num_checkpoints) {
            printf("Invalid location ID.\n");
            continue;
        }

        // Display available checkpoints for this round
        display_available_checkpoints(g, round_idx);

        // --- Step 4: Visitor Chooses First ---
        printf("\nEnter the Checkpoint ID you wish to visit (or -1 for automatic suggestion): ");
        int chosen_id = -1;
        if (scanf("%d", &chosen_id) != 1) {
            chosen_id = -1;
        }

        int target_id = -1;
        int interim_id = -1;
        int return_round = -1;

        if (chosen_id >= 0 && chosen_id < g->num_checkpoints && g->nodes[chosen_id].num_rounds > 0 && chosen_id != current_cp) {
            int avail = g->nodes[chosen_id].rounds[round_idx].available_seats;
            int max_s = g->nodes[chosen_id].rounds[round_idx].max_seats;

            if (avail >= group_size) {
                // Room has space!
                target_id = chosen_id;
                printf("\n========================================================\n");
                printf("             VISITOR SELECTION CONFIRMED                \n");
                printf("========================================================\n");
                printf(" Selected Target : %s (ID: %d)\n", g->nodes[target_id].name, target_id);
                printf(" Room Number     : %d\n", g->nodes[target_id].room_num);
                printf(" Seats Remaining : %d / %d (after your visit)\n", avail - group_size, max_s);
                printf("========================================================\n");
            } else {
                // Room is at MAXIMUM CAPACITY right now in this round!
                printf("\n========================================================\n");
                printf(" [ROOM FULL] %s (Room %d) is at MAX CAPACITY in Round %d!\n",
                       g->nodes[chosen_id].name, g->nodes[chosen_id].room_num, round_choice);
                printf(" Available Seats : %d / %d  (Your group size: %d)\n", avail, max_s, group_size);
                printf("========================================================\n");

                // Check if this room has available seats in a subsequent round
                for (int r = round_idx + 1; r < g->nodes[chosen_id].num_rounds; r++) {
                    if (g->nodes[chosen_id].rounds[r].available_seats >= group_size) {
                        return_round = r;
                        break;
                    }
                }

                if (return_round != -1) {
                    // Find an interim checkpoint to visit first before returning
                    Recommendation interim_rec = recommend_interim_checkpoint(g, current_cp, chosen_id, group_size, round_idx);
                    if (interim_rec.next_checkpoint_id != -1) {
                        interim_id = interim_rec.next_checkpoint_id;

                        printf("\n>>> SMART ITINERARY PLAN: VISIT BEFORE RETURNING <<<\n");
                        printf("While waiting for '%s' in Round %d, we recommend visiting:\n\n",
                               g->nodes[chosen_id].name, return_round + 1);
                        printf(" • STEP 1 (Round %d): Visit %s (ID: %d, Room %d)\n",
                               round_choice,
                               g->nodes[interim_id].name,
                               interim_id,
                               g->nodes[interim_id].room_num);
                        printf("   Seats Available  : %d / %d\n",
                               g->nodes[interim_id].rounds[round_idx].available_seats,
                               g->nodes[interim_id].rounds[round_idx].max_seats);
                        printf(" • STEP 2 (Round %d): Return to your desired %s (ID: %d, Room %d)!\n",
                               return_round + 1,
                               g->nodes[chosen_id].name,
                               chosen_id,
                               g->nodes[chosen_id].room_num);
                        printf("   Seats Available  : %d / %d\n",
                               g->nodes[chosen_id].rounds[return_round].available_seats,
                               g->nodes[chosen_id].rounds[return_round].max_seats);
                        printf("--------------------------------------------------------\n");

                        printf("Options:\n");
                        printf(" [1] Accept Smart Plan (Go to %s in Round %d, return to %s in Round %d)\n",
                               g->nodes[interim_id].name, round_choice, g->nodes[chosen_id].name, return_round + 1);
                        printf(" [2] Choose a different checkpoint\n");
                        printf("Select: ");

                        int plan_choice = 1;
                        if (scanf("%d", &plan_choice) == 1 && plan_choice == 1) {
                            target_id = interim_id;
                        } else {
                            continue;
                        }
                    }
                } else {
                    printf("\n[Notice] '%s' has no available seats in later rounds.\n", g->nodes[chosen_id].name);
                    printf("Calculating best available alternative for Round %d...\n", round_choice);
                }
            }
        }

        // Automatic recommendation fallback if visitor didn't pick or if target not yet set
        if (target_id == -1) {
            printf("\n>>> Calculating optimal recommendation for group of %d ...\n", group_size);
            Recommendation rec = recommend_next_checkpoint(g, current_cp, group_size, round_idx);

            if (rec.is_split) {
                printf("\n========================================================\n");
                printf(" [NOTICE] GROUP SPLIT RECOMMENDATION                   \n");
                printf("========================================================\n");
                printf("Your group size (%d) exceeds the free seats at any single room.\n", group_size);
                printf("To avoid delays and overcrowding, we recommend splitting:\n");
                printf(" • Subgroup 1 (%d visitors) -> %s (Room %d)\n",
                       rec.split_info.cp1_group_size,
                       g->nodes[rec.split_info.cp1_id].name,
                       g->nodes[rec.split_info.cp1_id].room_num);
                printf(" • Subgroup 2 (%d visitors) -> %s (Room %d)\n",
                       rec.split_info.cp2_group_size,
                       g->nodes[rec.split_info.cp2_id].name,
                       g->nodes[rec.split_info.cp2_id].room_num);
                printf("--------------------------------------------------------\n");
                continue;
            }

            if (rec.next_checkpoint_id == -1) {
                printf("\n[Notice] No available checkpoints have enough seats for your group of %d in Round %d.\n",
                       group_size, round_choice);
                continue;
            }

            target_id = rec.next_checkpoint_id;
            int seats_after = g->nodes[target_id].rounds[round_idx].available_seats - group_size;
            if (seats_after < 0) seats_after = 0;

            printf("\n========================================================\n");
            printf("             RECOMMENDED NEXT CHECKPOINT                \n");
            printf("========================================================\n");
            printf(" Recommended Target : %s (ID: %d)\n", g->nodes[target_id].name, target_id);
            printf(" Room Number        : %d\n", g->nodes[target_id].room_num);
            printf(" Seats Remaining    : %d / %d (after your visit)\n", 
                   seats_after, g->nodes[target_id].rounds[round_idx].max_seats);
            printf(" System Cost Score  : %.2f (Distance + Congestion balanced)\n", rec.score);
            printf("========================================================\n");
        }

        // --- Step 5: Route Guidance ---
        int want_route = 1;
        printf("\nDo you want step-by-step route guidance to this checkpoint?\n");
        printf(" [1] Yes (Show shortest walking route)\n");
        printf(" [2] No\n");
        printf("Select: ");
        if (scanf("%d", &want_route) == 1 && want_route == 1) {
            Path nav_path;
            if (dijkstra_shortest_path(g, current_cp, target_id, NULL, 0, &nav_path)) {
                printf("\n--- STEP-BY-STEP WALKING ROUTE ---\n");
                printf("Route: ");
                for (int i = 0; i < nav_path.node_count; i++) {
                    int nid = nav_path.nodes[i];
                    printf("[%s]", g->nodes[nid].name);
                    if (i < nav_path.node_count - 1) {
                        printf(" ──> ");
                    }
                }
                printf("\nTotal Distance: %.1f meters\n", nav_path.total_distance);
            } else {
                printf("No direct walking route found between these locations.\n");
            }
        }

        // --- Step 6: Confirmation & Persistence ---
        int confirm_booking = 0;
        printf("\nConfirm and book participation for this checkpoint round?\n");
        printf(" [1] Confirm & Save Booking\n");
        printf(" [2] Skip\n");
        printf("Select: ");
        if (scanf("%d", &confirm_booking) == 1 && confirm_booking == 1) {
            // Book current target for round_idx
            g->nodes[target_id].rounds[round_idx].available_seats -= group_size;
            if (g->nodes[target_id].rounds[round_idx].available_seats < 0) {
                g->nodes[target_id].rounds[round_idx].available_seats = 0;
            }
            g->nodes[target_id].available_seats = g->nodes[target_id].rounds[round_idx].available_seats;

            // If an interim plan was accepted, also book the return checkpoint for return_round!
            if (interim_id != -1 && return_round != -1 && chosen_id != -1) {
                g->nodes[chosen_id].rounds[return_round].available_seats -= group_size;
                if (g->nodes[chosen_id].rounds[return_round].available_seats < 0) {
                    g->nodes[chosen_id].rounds[return_round].available_seats = 0;
                }
                printf("\n[SUCCESS] Confirmed Step 1: Reserved %d seat(s) at %s (Round %d).\n",
                       group_size, g->nodes[target_id].name, round_choice);
                printf("[SUCCESS] Confirmed Step 2: Reserved %d seat(s) at %s (Round %d) when you return!\n",
                       group_size, g->nodes[chosen_id].name, return_round + 1);
            } else {
                printf("\n[SUCCESS] Confirmed! %d seat(s) reserved for %s (Round %d).\n",
                       group_size, g->nodes[target_id].name, round_choice);
            }

            if (save_checkpoints("checkpoints.txt", g)) {
                printf("Data successfully saved to 'checkpoints.txt'.\n");
            } else {
                printf("\n[Warning] Reservation held in memory, but could not write to 'checkpoints.txt'.\n");
            }
        }
    }

    free_graph(g);
    return 0;
}
