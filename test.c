#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "graph.h"

// Recommendation function structure and prototype from recommendation.c
typedef struct {
    int next_checkpoint_id;
    int selected_path_index;
    double score;
} Recommendation;

Recommendation recommend_next_checkpoint(Graph* g, int current_checkpoint, int group_size);

int main() {
    // 1. System Setup
    int total_checkpoints = 4;
    Graph* g = create_graph(total_checkpoints);

    printf("====================================================\n");
    printf("   KVIS OPEN HOUSE SMART MANAGEMENT SYSTEM (C)      \n");
    printf("====================================================\n\n");

    // 2. Staff Setup Phase (Adding/Configuring Checkpoints)
    printf("--- [STAFF MODE] Setting up Checkpoints ---\n");
    
    // Checkpoint 0
    snprintf(g->nodes[0].name, 50, "Main Entrance");
    g->nodes[0].available_seats = 100;
    g->nodes[0].max_seats = 100;

    // Checkpoint 1
    snprintf(g->nodes[1].name, 50, "Robotics Lab");
    g->nodes[1].available_seats = 2;   // High congestion (Only 2 seats!)
    g->nodes[1].max_seats = 20;

    // Checkpoint 2
    snprintf(g->nodes[2].name, 50, "Chemistry Exhibition");
    g->nodes[2].available_seats = 15;  // Low congestion
    g->nodes[2].max_seats = 20;

    // Checkpoint 3
    snprintf(g->nodes[3].name, 50, "Astronomy Dome");
    g->nodes[3].available_seats = 25;
    g->nodes[3].max_seats = 30;

    // Connecting paths between checkpoints (u, v, distance in meters)
    add_edge(g, 0, 1, 50.0);  // Entrance -> Robotics (50m)
    add_edge(g, 0, 2, 80.0);  // Entrance -> Chemistry (80m)
    add_edge(g, 1, 3, 40.0);  // Robotics -> Astronomy (40m)
    add_edge(g, 2, 3, 30.0);  // Chemistry -> Astronomy (30m)
    add_edge(g, 1, 2, 20.0);  // Robotics -> Chemistry (20m)

    printf("Staff Configuration Loaded Successfully:\n");
    for (int i = 0; i < total_checkpoints; i++) {
        printf(" ID %d: %-22s | Free Seats: %2d/%2d\n", 
               g->nodes[i].id, g->nodes[i].name, 
               g->nodes[i].available_seats, g->nodes[i].max_seats);
    }
    printf("----------------------------------------------------\n\n");

    // 3. Visitor Interaction Phase
    int current_cp, group_size, guidance_mode;

    printf("--- [VISITOR MODE] Interactive Guidance ---\n");
    printf("Enter your current Checkpoint ID (0-3): ");
    scanf("%d", &current_cp);

    printf("Enter your group size: ");
    scanf("%d", &group_size);

    // Run Recommendation Engine using Eppstein's Algorithm
    Recommendation rec = recommend_next_checkpoint(g, current_cp, group_size);

    if (rec.next_checkpoint_id == -1) {
        printf("\nSorry, no available checkpoints can accommodate your group size of %d.\n", group_size);
        free(g);
        return 0;
    }

    printf("\n>>> RECOMMENDATION RESULT <<<\n");
    printf("Suggested Next Checkpoint: %s (ID: %d)\n", 
           g->nodes[rec.next_checkpoint_id].name, rec.next_checkpoint_id);

    // 4. Path Guidance Selection
    printf("\nDo you want automatic route guidance?\n");
    printf(" [1] Yes (Use System Optimized Path Guidance)\n");
    printf(" [2] No  (Explore manually)\n");
    printf("Selection: ");
    scanf("%d", &guidance_mode);

    if (guidance_mode == 1) {
        Path candidate_paths[5];
        int path_count = 0;
        
        // Fetch paths via Eppstein's Algorithm
        eppstein_k_shortest_paths(g, current_cp, rec.next_checkpoint_id, 5, candidate_paths, &path_count);

        printf("\n--- [PATH GUIDANCE ROUTE] ---\n");
        printf("Route: ");
        for (int i = 0; i < candidate_paths[rec.selected_path_index].node_count; i++) {
            int node_id = candidate_paths[rec.selected_path_index].nodes[i];
            printf("[%s]", g->nodes[node_id].name);
            if (i < candidate_paths[rec.selected_path_index].node_count - 1) {
                printf(" -> ");
            }
        }
        printf("\nTotal Distance: %.1f meters\n", candidate_paths[rec.selected_path_index].total_distance);
        printf("Enjoy your visit!\n");
    } else {
        printf("\nManual mode selected. Refer to the venue floor map for directions.\n");
    }

    // Cleanup
    for (int i = 0; i < g->num_checkpoints; i++) {
        Edge* curr = g->nodes[i].head;
        while (curr) {
            Edge* tmp = curr;
            curr = curr->next;
            free(tmp);
        }
    }
    free(g);

    return 0;
}