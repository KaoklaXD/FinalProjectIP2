#include <stdio.h>
#include <float.h>
#include "graph.h"

#define K_PATHS 5
#define ALPHA 1.0  // Distance weight factor
#define BETA 50.0  // Seat congestion penalty weight factor

typedef struct Recommendation {
    int next_checkpoint_id;
    int selected_path_index;
    double score;
} Recommendation;

// Calculates fitness score for a path based on distance, group size, and availability
double calculate_path_score(Graph* g, Path path, int group_size) {
    int target_node = path.nodes[path.node_count - 1];
    Checkpoint cp = g->nodes[target_node];

    // Double-check seat availability safety constraint
    if (cp.available_seats < group_size || cp.max_seats <= 0) {
        return INF; 
    }

    // Congestion Ratio (0 = empty, 1 = full)
    double occupancy = 1.0 - ((double)(cp.available_seats - group_size) / (double)cp.max_seats);
    
    // Total Cost = Distance Weight + Congestion Weight
    double score = (ALPHA * path.total_distance) + (BETA * occupancy);
    return score;
}

// "Recommend Next Checkpoint" with strict capacity filtering
Recommendation recommend_next_checkpoint(Graph* g, int current_checkpoint, int group_size) {
    Recommendation best_rec = {-1, -1, INF};

    for (int target = 0; target < g->num_checkpoints; target++) {
        // 1. Skip current location
        if (target == current_checkpoint) continue;

        // 2. HARD FILTER: Immediately reject checkpoints without enough seats for group_size
        if (g->nodes[target].available_seats < group_size) {
            continue;
        }

        Path candidate_paths[K_PATHS];
        int path_count = 0;

        // Run Eppstein's algorithm to obtain candidate paths to target checkpoint
        eppstein_k_shortest_paths(g, current_checkpoint, target, K_PATHS, candidate_paths, &path_count);

        for (int p = 0; p < path_count; p++) {
            double score = calculate_path_score(g, candidate_paths[p], group_size);
            
            // Only update if score is strictly better AND valid (< INF)
            if (score < best_rec.score && score < INF) {
                best_rec.score = score;
                best_rec.next_checkpoint_id = target;
                best_rec.selected_path_index = p;
            }
        }
    }

    return best_rec;
}