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

    // Penalty check: group size larger than capacity
    if (cp.available_seats < group_size) {
        return INF; 
    }

    // Congestion Ratio (0 = empty, 1 = full)
    double occupancy = 1.0 - ((double)(cp.available_seats - group_size) / (double)cp.max_seats);
    
    // Total Cost = Distance Weight + Congestion Weight
    double score = (ALPHA * path.total_distance) + (BETA * occupancy);
    return score;
}

// "Recommend Next Checkpoint" & "Path Guidance" Logic
Recommendation recommend_next_checkpoint(Graph* g, int current_checkpoint, int group_size) {
    Recommendation best_rec = {-1, -1, DBL_MAX};

    for (int target = 0; target < g->num_checkpoints; target++) {
        if (target == current_checkpoint) continue;

        Path candidate_paths[K_PATHS];
        int path_count = 0;

        // Run Eppstein's algorithm to obtain candidate paths to target checkpoint
        eppstein_k_shortest_paths(g, current_checkpoint, target, K_PATHS, candidate_paths, &path_count);

        for (int p = 0; p < path_count; p++) {
            double score = calculate_path_score(g, candidate_paths[p], group_size);
            if (score < best_rec.score) {
                best_rec.score = score;
                best_rec.next_checkpoint_id = target;
                best_rec.selected_path_index = p;
            }
        }
    }

    return best_rec;
}