#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <float.h>
#include "graph.h"
#include "storage.h"
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

#define HUNGARIAN_PENALTY_INF 1e6

// Kuhn-Munkres (Hungarian Algorithm) O(N^3) implementation
static double solve_hungarian_matrix(int n, double cost[MAX_ITINERARY_SLOTS + 1][MAX_ITINERARY_SLOTS + 1], int match_row_of_col[MAX_ITINERARY_SLOTS + 1]) {
    double u[MAX_ITINERARY_SLOTS + 1] = {0};
    double v[MAX_ITINERARY_SLOTS + 1] = {0};
    int p[MAX_ITINERARY_SLOTS + 1] = {0};
    int way[MAX_ITINERARY_SLOTS + 1] = {0};

    for (int i = 1; i <= n; i++) {
        p[0] = i;
        int j0 = 0;
        double minv[MAX_ITINERARY_SLOTS + 1];
        char used[MAX_ITINERARY_SLOTS + 1] = {0};
        for (int j = 0; j <= n; j++) {
            minv[j] = 1e9;
        }

        do {
            used[j0] = 1;
            int i0 = p[j0];
            int j1 = 0;
            double delta = 1e9;
            for (int j = 1; j <= n; j++) {
                if (!used[j]) {
                    double cur = cost[i0][j] - u[i0] - v[j];
                    if (cur < minv[j]) {
                        minv[j] = cur;
                        way[j] = j0;
                    }
                    if (minv[j] < delta) {
                        delta = minv[j];
                        j1 = j;
                    }
                }
            }
            for (int j = 0; j <= n; j++) {
                if (used[j]) {
                    u[p[j]] += delta;
                    v[j] -= delta;
                } else {
                    minv[j] -= delta;
                }
            }
            j0 = j1;
        } while (p[j0] != 0);

        do {
            int j1 = way[j0];
            p[j0] = p[j1];
            j0 = j1;
        } while (j0 != 0);
    }

    for (int j = 1; j <= n; j++) {
        match_row_of_col[j] = p[j];
    }
    return -v[0];
}

ItinerarySchedule hungarian_optimize_itinerary(
    int k,
    const int* requested_checkpoints,
    int group_size,
    int num_slots,
    const int* activity_round_indices,
    const int capacity_matrix[MAX_ITINERARY_SLOTS][MAX_ITINERARY_SLOTS]
) {
    ItinerarySchedule sched;
    sched.is_valid = 0;
    sched.spearman_penalty = 0.0;
    for (int s = 0; s < MAX_ITINERARY_SLOTS; s++) {
        sched.assigned_checkpoint_id[s] = -1;
        sched.slot_round_idx[s] = (s < num_slots) ? activity_round_indices[s] : -1;
    }

    if (k <= 0 || num_slots <= 0 || k > num_slots) {
        return sched;
    }

    int n = num_slots;
    double cost[MAX_ITINERARY_SLOTS + 1][MAX_ITINERARY_SLOTS + 1];
    memset(cost, 0, sizeof(cost));

    for (int i = 1; i <= n; i++) {
        for (int j = 1; j <= n; j++) {
            if (i <= k) {
                int chk_idx = i - 1;
                int slot_idx = j - 1;
                if (capacity_matrix[chk_idx][slot_idx] < group_size) {
                    cost[i][j] = HUNGARIAN_PENALTY_INF;
                } else {
                    cost[i][j] = fabs((double)(chk_idx - slot_idx));
                }
            } else {
                cost[i][j] = 0.0; // Dummy row for unassigned slots
            }
        }
    }

    int match_row_of_col[MAX_ITINERARY_SLOTS + 1] = {0};
    solve_hungarian_matrix(n, cost, match_row_of_col);

    double total_penalty = 0.0;
    int valid = 1;

    for (int j = 1; j <= n; j++) {
        int row = match_row_of_col[j];
        int slot_idx = j - 1;
        if (row <= k) {
            int chk_idx = row - 1;
            if (cost[row][j] >= HUNGARIAN_PENALTY_INF / 2.0) {
                valid = 0; // Capacity was violated
            }
            sched.assigned_checkpoint_id[slot_idx] = requested_checkpoints[chk_idx];
            total_penalty += fabs((double)(chk_idx - slot_idx));
        } else {
            sched.assigned_checkpoint_id[slot_idx] = -1;
        }
    }

    sched.is_valid = valid;
    sched.spearman_penalty = total_penalty;
    return sched;
}

typedef struct DFSContext {
    int k;
    const int* requested_checkpoints;
    int group_size;
    int num_slots;
    const int* activity_round_indices;
    const int (*capacity_matrix)[MAX_ITINERARY_SLOTS];
    int max_options;
    int options_count;
    ItinerarySchedule options[MAX_ALTERNATIVE_ITINERARIES];
    int slot_assigned_to[MAX_ITINERARY_SLOTS];
    int slot_used[MAX_ITINERARY_SLOTS];
} DFSContext;

static void dfs_search(DFSContext* ctx, int depth, double current_penalty) {
    if (depth == ctx->k) {
        ItinerarySchedule candidate;
        candidate.is_valid = 1;
        candidate.spearman_penalty = current_penalty;
        for (int s = 0; s < MAX_ITINERARY_SLOTS; s++) {
            if (s < ctx->num_slots) {
                candidate.assigned_checkpoint_id[s] = ctx->slot_assigned_to[s];
                candidate.slot_round_idx[s] = ctx->activity_round_indices[s];
            } else {
                candidate.assigned_checkpoint_id[s] = -1;
                candidate.slot_round_idx[s] = -1;
            }
        }

        // Check for duplicate in options
        for (int opt = 0; opt < ctx->options_count; opt++) {
            int identical = 1;
            for (int s = 0; s < ctx->num_slots; s++) {
                if (ctx->options[opt].assigned_checkpoint_id[s] != candidate.assigned_checkpoint_id[s]) {
                    identical = 0;
                    break;
                }
            }
            if (identical) return;
        }

        // Insert sorted by penalty ascending
        int insert_pos = ctx->options_count;
        while (insert_pos > 0 && ctx->options[insert_pos - 1].spearman_penalty > candidate.spearman_penalty) {
            if (insert_pos < ctx->max_options) {
                ctx->options[insert_pos] = ctx->options[insert_pos - 1];
            }
            insert_pos--;
        }
        if (insert_pos < ctx->max_options) {
            ctx->options[insert_pos] = candidate;
            if (ctx->options_count < ctx->max_options) {
                ctx->options_count++;
            }
        }
        return;
    }

    // Bound Prune: if options list is full and current_penalty >= worst penalty, stop exploring
    if (ctx->options_count == ctx->max_options && current_penalty >= ctx->options[ctx->max_options - 1].spearman_penalty) {
        return;
    }

    int chk_idx = depth;
    for (int j = 0; j < ctx->num_slots; j++) {
        if (ctx->slot_used[j]) continue;

        // Capacity Prune: abort immediately if room does not have enough seats in this slot
        if (ctx->capacity_matrix[chk_idx][j] < ctx->group_size) {
            continue;
        }

        double step_penalty = fabs((double)(chk_idx - j));
        double new_penalty = current_penalty + step_penalty;

        // Bound Prune: abort if penalty already exceeds bound
        if (ctx->options_count == ctx->max_options && new_penalty >= ctx->options[ctx->max_options - 1].spearman_penalty) {
            continue;
        }

        ctx->slot_used[j] = 1;
        ctx->slot_assigned_to[j] = ctx->requested_checkpoints[chk_idx];

        dfs_search(ctx, depth + 1, new_penalty);

        ctx->slot_used[j] = 0;
        ctx->slot_assigned_to[j] = -1;
    }
}

int dfs_top_itineraries(
    int k,
    const int* requested_checkpoints,
    int group_size,
    int num_slots,
    const int* activity_round_indices,
    const int capacity_matrix[MAX_ITINERARY_SLOTS][MAX_ITINERARY_SLOTS],
    int max_options,
    ItinerarySchedule options[MAX_ALTERNATIVE_ITINERARIES]
) {
    if (k <= 0 || num_slots <= 0 || k > num_slots || max_options <= 0) {
        return 0;
    }
    if (max_options > MAX_ALTERNATIVE_ITINERARIES) {
        max_options = MAX_ALTERNATIVE_ITINERARIES;
    }

    DFSContext ctx;
    ctx.k = k;
    ctx.requested_checkpoints = requested_checkpoints;
    ctx.group_size = group_size;
    ctx.num_slots = num_slots;
    ctx.activity_round_indices = activity_round_indices;
    ctx.capacity_matrix = capacity_matrix;
    ctx.max_options = max_options;
    ctx.options_count = 0;
    for (int s = 0; s < MAX_ITINERARY_SLOTS; s++) {
        ctx.slot_assigned_to[s] = -1;
        ctx.slot_used[s] = 0;
    }

    dfs_search(&ctx, 0, 0.0);

    for (int opt = 0; opt < ctx.options_count; opt++) {
        options[opt] = ctx.options[opt];
    }
    return ctx.options_count;
}

int optimize_itineraries_in_ram(
    Graph* g,
    int k,
    const int* requested_checkpoints,
    int group_size,
    ItinerarySchedule* optimal_schedule,
    ItinerarySchedule alternatives[MAX_ALTERNATIVE_ITINERARIES],
    int* num_alternatives
) {
    if (!g || k <= 0 || !requested_checkpoints || !optimal_schedule || !alternatives || !num_alternatives) {
        return 0;
    }

    int act_rounds[MAX_ITINERARY_SLOTS];
    int num_slots = get_activity_rounds(g, act_rounds, MAX_ITINERARY_SLOTS);
    if (num_slots <= 0 || k > num_slots) {
        return 0;
    }

    // Step 1: In-memory capacity extraction ("Never loop your database")
    int capacity_matrix[MAX_ITINERARY_SLOTS][MAX_ITINERARY_SLOTS];
    memset(capacity_matrix, 0, sizeof(capacity_matrix));

    for (int i = 0; i < k; i++) {
        int cp_id = requested_checkpoints[i];
        if (cp_id < 0 || cp_id >= g->num_checkpoints) {
            return 0;
        }
        for (int s = 0; s < num_slots; s++) {
            int r_idx = act_rounds[s];
            if (r_idx >= 0 && r_idx < g->nodes[cp_id].num_rounds) {
                capacity_matrix[i][s] = g->nodes[cp_id].rounds[r_idx].available_seats;
            } else {
                capacity_matrix[i][s] = 0;
            }
        }
    }

    // Step 2: Hungarian algorithm for global minimum Spearman penalty
    *optimal_schedule = hungarian_optimize_itinerary(
        k, requested_checkpoints, group_size, num_slots, act_rounds, capacity_matrix
    );

    // Step 3: DFS with Branch-and-Bound Pruning for Top-M alternative itineraries
    *num_alternatives = dfs_top_itineraries(
        k, requested_checkpoints, group_size, num_slots, act_rounds, capacity_matrix,
        MAX_ALTERNATIVE_ITINERARIES, alternatives
    );

    return (optimal_schedule->is_valid || *num_alternatives > 0);
}

int hungarian_suggest_alternative(Graph* g, int current_loc, int round_idx, int rejected_target, int group_size, const int* excluded_checkpoints, int num_excluded) {
    if (!g || round_idx < 0) return -1;

    int candidates[MAX_CHECKPOINTS];
    int cand_count = 0;
    for (int i = 0; i < g->num_checkpoints; i++) {
        if (i == 0 || i == rejected_target || g->nodes[i].num_rounds <= 0) continue;
        if (round_idx >= g->nodes[i].num_rounds) continue;
        if (g->nodes[i].rounds[round_idx].is_lunch_break) continue;

        // Check if excluded / already visited in other rounds
        int is_excluded = 0;
        if (excluded_checkpoints && num_excluded > 0) {
            for (int e = 0; e < num_excluded; e++) {
                if (excluded_checkpoints[e] == i) {
                    is_excluded = 1;
                    break;
                }
            }
        }
        if (is_excluded) continue;

        int avail = g->nodes[i].rounds[round_idx].available_seats;
        if (avail >= group_size) {
            candidates[cand_count++] = i;
        }
    }

    if (cand_count == 0) return -1;
    if (cand_count == 1) return candidates[0];

    int m = cand_count;
    if (m > MAX_ITINERARY_SLOTS) m = MAX_ITINERARY_SLOTS;

    double cost[MAX_ITINERARY_SLOTS + 1][MAX_ITINERARY_SLOTS + 1];
    memset(cost, 0, sizeof(cost));

    for (int j = 1; j <= m; j++) {
        int target = candidates[j - 1];
        Path p;
        double dist = 100.0;
        if (dijkstra_shortest_path(g, current_loc, target, NULL, 0, &p)) {
            dist = p.total_distance;
        }
        int avail = g->nodes[target].rounds[round_idx].available_seats;
        int max_s = g->nodes[target].rounds[round_idx].max_seats;
        double occ = 1.0 - ((double)(avail - group_size) / (double)max_s);
        cost[1][j] = (ALPHA * dist) + (BETA * occ);
    }
    for (int i = 2; i <= m; i++) {
        for (int j = 1; j <= m; j++) {
            cost[i][j] = 0.0;
        }
    }

    int match_row_of_col[MAX_ITINERARY_SLOTS + 1] = {0};
    solve_hungarian_matrix(m, cost, match_row_of_col);

    for (int j = 1; j <= m; j++) {
        if (match_row_of_col[j] == 1) {
            return candidates[j - 1];
        }
    }

    return candidates[0];
}

typedef struct TourSearchContext {
    Graph* g;
    int start_node;
    const int* target_nodes;
    int count;
    double dist_start[MAX_CHECKPOINTS];
    double dist_pair[MAX_CHECKPOINTS][MAX_CHECKPOINTS];
    int best_order[MAX_CHECKPOINTS];
    double best_distance;
    int current_order[MAX_CHECKPOINTS];
    int used[MAX_CHECKPOINTS];
} TourSearchContext;

static void permute_tour(TourSearchContext* ctx, int depth, int prev_idx, double current_distance) {
    if (current_distance >= ctx->best_distance) {
        return;
    }

    if (depth == ctx->count) {
        ctx->best_distance = current_distance;
        for (int i = 0; i < ctx->count; i++) {
            ctx->best_order[i] = ctx->current_order[i];
        }
        return;
    }

    for (int i = 0; i < ctx->count; i++) {
        if (!ctx->used[i]) {
            int next_node = ctx->target_nodes[i];
            double leg_dist = (depth == 0) ? ctx->dist_start[i] : ctx->dist_pair[prev_idx][i];
            if (current_distance + leg_dist >= ctx->best_distance) {
                continue;
            }

            ctx->used[i] = 1;
            ctx->current_order[depth] = next_node;
            permute_tour(ctx, depth + 1, i, current_distance + leg_dist);
            ctx->used[i] = 0;
        }
    }
}

OptimalTour find_optimal_tour(Graph* g, int start_node, const int* target_checkpoints, int count) {
    OptimalTour tour;
    tour.count = 0;
    tour.total_distance = 0.0;
    for (int i = 0; i < MAX_CHECKPOINTS; i++) {
        tour.ordered_checkpoints[i] = -1;
    }

    if (!g || count <= 0 || !target_checkpoints) {
        return tour;
    }

    TourSearchContext ctx;
    ctx.g = g;
    ctx.start_node = start_node;
    ctx.target_nodes = target_checkpoints;
    ctx.count = count;
    ctx.best_distance = INF;
    for (int i = 0; i < count; i++) {
        ctx.used[i] = 0;
        ctx.best_order[i] = target_checkpoints[i];
    }

    for (int i = 0; i < count; i++) {
        Path p;
        if (dijkstra_shortest_path(g, start_node, target_checkpoints[i], NULL, 0, &p)) {
            ctx.dist_start[i] = p.total_distance;
        } else {
            ctx.dist_start[i] = 1000.0;
        }
    }

    for (int i = 0; i < count; i++) {
        for (int j = 0; j < count; j++) {
            if (i == j) {
                ctx.dist_pair[i][j] = 0.0;
            } else {
                Path p;
                if (dijkstra_shortest_path(g, target_checkpoints[i], target_checkpoints[j], NULL, 0, &p)) {
                    ctx.dist_pair[i][j] = p.total_distance;
                } else {
                    ctx.dist_pair[i][j] = 1000.0;
                }
            }
        }
    }

    permute_tour(&ctx, 0, -1, 0.0);

    tour.count = count;
    tour.total_distance = 0.0;
    int prev = start_node;
    for (int i = 0; i < count; i++) {
        tour.ordered_checkpoints[i] = ctx.best_order[i];
        dijkstra_shortest_path(g, prev, ctx.best_order[i], NULL, 0, &tour.leg_paths[i]);
        tour.total_distance += tour.leg_paths[i].total_distance;
        prev = ctx.best_order[i];
    }

    return tour;
}

int checkpoint_has_available_round(Graph* g, int cp_id, int group_size) {
    if (!g || cp_id < 0 || cp_id >= g->num_checkpoints || group_size <= 0) {
        return 0;
    }
    int act_rounds[MAX_ITINERARY_SLOTS];
    int num_slots = get_activity_rounds(g, act_rounds, MAX_ITINERARY_SLOTS);
    for (int s = 0; s < num_slots; s++) {
        int r_idx = act_rounds[s];
        if (r_idx >= 0 && r_idx < g->nodes[cp_id].num_rounds) {
            if (!g->nodes[cp_id].rounds[r_idx].is_lunch_break &&
                g->nodes[cp_id].rounds[r_idx].available_seats >= group_size) {
                return 1;
            }
        }
    }
    return 0;
}

int is_exact_order_feasible(Graph* g, int k, const int* requested_checkpoints, int group_size) {
    if (!g || k <= 0 || !requested_checkpoints || group_size <= 0) {
        return 0;
    }
    int act_rounds[MAX_ITINERARY_SLOTS];
    int num_slots = get_activity_rounds(g, act_rounds, MAX_ITINERARY_SLOTS);
    if (k > num_slots) {
        return 0;
    }
    for (int i = 0; i < k; i++) {
        int cp_id = requested_checkpoints[i];
        if (cp_id < 0 || cp_id >= g->num_checkpoints) {
            return 0;
        }
        int r_idx = act_rounds[i];
        if (r_idx < 0 || r_idx >= g->nodes[cp_id].num_rounds) {
            return 0;
        }
        if (g->nodes[cp_id].rounds[r_idx].is_lunch_break ||
            g->nodes[cp_id].rounds[r_idx].available_seats < group_size) {
            return 0;
        }
    }
    return 1;
}