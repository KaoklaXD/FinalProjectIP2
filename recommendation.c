#include <stdio.h>
#include <float.h>
#include "graph.h"
#include "recommendation.h"

#define K_PATHS 5
#define ALPHA 1.0   // Distance weight factor (meters)
#define BETA 50.0   // Seat congestion penalty weight factor

// Calculates fitness score for a path based on distance, group size, and availability
double calculate_path_score(Graph* g, Path path, int group_size, int round_idx) {
    if (!g || path.node_count <= 0) return INF;
    int target_node = path.nodes[path.node_count - 1];
    if (target_node < 0 || target_node >= g->num_checkpoints) return INF;

    Checkpoint cp = g->nodes[target_node];
    int avail = cp.available_seats;
    int max_s = cp.max_seats;

    if (round_idx >= 0 && round_idx < cp.num_rounds) {
        avail = cp.rounds[round_idx].available_seats;
        max_s = cp.rounds[round_idx].max_seats;
    }

    // Safety constraint: Must have enough available seats
    if (avail < group_size || max_s <= 0) {
        return INF; 
    }

    // Post-visit Congestion Ratio (0 = empty, 1 = full)
    double occupancy = 1.0 - ((double)(avail - group_size) / (double)max_s);
    if (occupancy < 0.0) occupancy = 0.0;
    if (occupancy > 1.0) occupancy = 1.0;
    
    // Total Cost = Distance Weight + Congestion Weight
    double score = (ALPHA * path.total_distance) + (BETA * occupancy);
    return score;
}

// "Recommend Next Checkpoint" with capacity filtering, round awareness, and split options
Recommendation recommend_next_checkpoint(Graph* g, int current_checkpoint, int group_size, int round_idx) {
    Recommendation best_rec;
    best_rec.is_split = 0;
    best_rec.next_checkpoint_id = -1;
    best_rec.selected_path_index = -1;
    best_rec.score = INF;
    best_rec.split_info.cp1_id = -1;
    best_rec.split_info.cp1_group_size = 0;
    best_rec.split_info.cp2_id = -1;
    best_rec.split_info.cp2_group_size = 0;
    best_rec.split_info.combined_score = INF;

    if (!g || current_checkpoint < 0 || current_checkpoint >= g->num_checkpoints) {
        return best_rec;
    }

    // Phase 1: Try to accommodate the entire group together
    for (int target = 0; target < g->num_checkpoints; target++) {
        if (target == current_checkpoint) continue;
        // Only consider valid activity checkpoints (ignore hallway waypoints and entrance)
        if (g->nodes[target].num_rounds <= 0 || target == 0) continue;

        int avail = g->nodes[target].available_seats;
        if (round_idx >= 0 && round_idx < g->nodes[target].num_rounds) {
            avail = g->nodes[target].rounds[round_idx].available_seats;
        }

        // HARD FILTER: Check if checkpoint can accommodate full group
        if (avail < group_size) {
            continue;
        }

        Path candidate_paths[K_PATHS];
        int path_count = 0;

        // Run K-shortest paths to obtain candidate routes to target checkpoint
        k_shortest_paths(g, current_checkpoint, target, K_PATHS, candidate_paths, &path_count);

        for (int p = 0; p < path_count; p++) {
            double score = calculate_path_score(g, candidate_paths[p], group_size, round_idx);
            
            if (score < best_rec.score && score < INF) {
                best_rec.score = score;
                best_rec.next_checkpoint_id = target;
                best_rec.selected_path_index = p;
            }
        }
    }

    // If an entire-group recommendation was found, return it
    if (best_rec.next_checkpoint_id != -1) {
        return best_rec;
    }

    // Phase 2: If group cannot fit in any single checkpoint, search for an optimal SplitOption
    if (group_size > 1) {
        double best_split_score = INF;
        SplitOption best_split = {-1, 0, -1, 0, INF};

        for (int s1 = 1; s1 < group_size; s1++) {
            int s2 = group_size - s1;

            for (int cp1 = 0; cp1 < g->num_checkpoints; cp1++) {
                if (cp1 == current_checkpoint || cp1 == 0 || g->nodes[cp1].num_rounds <= 0) continue;
                int avail1 = g->nodes[cp1].available_seats;
                if (round_idx >= 0 && round_idx < g->nodes[cp1].num_rounds) {
                    avail1 = g->nodes[cp1].rounds[round_idx].available_seats;
                }
                if (avail1 < s1) continue;

                Path p1;
                if (!dijkstra_shortest_path(g, current_checkpoint, cp1, NULL, 0, &p1)) continue;
                double score1 = calculate_path_score(g, p1, s1, round_idx);
                if (score1 >= INF) continue;

                for (int cp2 = cp1 + 1; cp2 < g->num_checkpoints; cp2++) {
                    if (cp2 == current_checkpoint || cp2 == 0 || g->nodes[cp2].num_rounds <= 0) continue;
                    int avail2 = g->nodes[cp2].available_seats;
                    if (round_idx >= 0 && round_idx < g->nodes[cp2].num_rounds) {
                        avail2 = g->nodes[cp2].rounds[round_idx].available_seats;
                    }
                    if (avail2 < s2) continue;

                    Path p2;
                    if (!dijkstra_shortest_path(g, current_checkpoint, cp2, NULL, 0, &p2)) continue;
                    double score2 = calculate_path_score(g, p2, s2, round_idx);
                    if (score2 >= INF) continue;

                    double combined = score1 + score2;
                    if (combined < best_split_score) {
                        best_split_score = combined;
                        best_split.cp1_id = cp1;
                        best_split.cp1_group_size = s1;
                        best_split.cp2_id = cp2;
                        best_split.cp2_group_size = s2;
                        best_split.combined_score = combined;
                    }
                }
            }
        }

        if (best_split.cp1_id != -1) {
            best_rec.is_split = 1;
            best_rec.split_info = best_split;
            best_rec.score = best_split_score;
        }
    }

    return best_rec;
}

// Backward-compatible fallback
Recommendation recommend_next_checkpoint_default(Graph* g, int current_checkpoint, int group_size) {
    return recommend_next_checkpoint(g, current_checkpoint, group_size, -1);
}

// Recommends an optimal interim checkpoint to visit during current_round before returning to desired_target in a later round
Recommendation recommend_interim_checkpoint(Graph* g, int current_cp, int desired_target, int group_size, int current_round) {
    Recommendation best_rec;
    best_rec.is_split = 0;
    best_rec.next_checkpoint_id = -1;
    best_rec.selected_path_index = -1;
    best_rec.score = INF;

    if (!g || current_cp < 0 || current_cp >= g->num_checkpoints ||
        desired_target < 0 || desired_target >= g->num_checkpoints) {
        return best_rec;
    }

    for (int interim = 0; interim < g->num_checkpoints; interim++) {
        if (interim == current_cp || interim == desired_target || interim == 0) continue;
        if (g->nodes[interim].num_rounds <= 0) continue;

        int avail = g->nodes[interim].available_seats;
        int max_s = g->nodes[interim].max_seats;
        if (current_round >= 0 && current_round < g->nodes[interim].num_rounds) {
            avail = g->nodes[interim].rounds[current_round].available_seats;
            max_s = g->nodes[interim].rounds[current_round].max_seats;
        }

        if (avail < group_size || max_s <= 0) continue;

        Path p_curr_to_interim, p_interim_to_target;
        if (!dijkstra_shortest_path(g, current_cp, interim, NULL, 0, &p_curr_to_interim)) continue;
        if (!dijkstra_shortest_path(g, interim, desired_target, NULL, 0, &p_interim_to_target)) continue;

        double occ = 1.0 - ((double)(avail - group_size) / (double)max_s);
        double total_dist = p_curr_to_interim.total_distance + p_interim_to_target.total_distance;
        double score = (ALPHA * total_dist) + (BETA * occ);

        if (score < best_rec.score) {
            best_rec.score = score;
            best_rec.next_checkpoint_id = interim;
            best_rec.selected_path_index = 0;
        }
    }

    return best_rec;
}