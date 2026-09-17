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

        // Display current overview
        display_available_checkpoints(g, round_idx);

        // --- Step 4: Run Smart Recommendation Engine ---
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
            printf("Please try choosing a different round or a smaller group size.\n");
            continue;
        }

        int target_id = rec.next_checkpoint_id;
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

        // --- Step 5: Route Guidance ---
        int want_route = 1;
        printf("\nDo you want step-by-step route guidance to this checkpoint?\n");
        printf(" [1] Yes (Show shortest walking route)\n");
        printf(" [2] No\n");
        printf("Select: ");
        if (scanf("%d", &want_route) == 1 && want_route == 1) {
            Path candidate_paths[3];
            int path_count = 0;
            k_shortest_paths(g, current_cp, target_id, 3, candidate_paths, &path_count);

            if (path_count > 0) {
                printf("\n--- STEP-BY-STEP WALKING ROUTE ---\n");
                printf("Route: ");
                for (int i = 0; i < candidate_paths[0].node_count; i++) {
                    int nid = candidate_paths[0].nodes[i];
                    printf("[%s]", g->nodes[nid].name);
                    if (i < candidate_paths[0].node_count - 1) {
                        printf(" ──> ");
                    }
                }
                printf("\nTotal Distance: %.1f meters\n", candidate_paths[0].total_distance);
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
            g->nodes[target_id].rounds[round_idx].available_seats -= group_size;
            if (g->nodes[target_id].rounds[round_idx].available_seats < 0) {
                g->nodes[target_id].rounds[round_idx].available_seats = 0;
            }
            g->nodes[target_id].available_seats = g->nodes[target_id].rounds[round_idx].available_seats;

            if (save_checkpoints("checkpoints.txt", g)) {
                printf("\n[SUCCESS] Confirmed! %d seat(s) reserved for %s (Round %d).\n",
                       group_size, g->nodes[target_id].name, round_choice);
                printf("Data successfully saved to 'checkpoints.txt'.\n");
            } else {
                printf("\n[Warning] Reservation held in memory, but could not write to 'checkpoints.txt'.\n");
            }
        }
    }

    free_graph(g);
    return 0;
}
