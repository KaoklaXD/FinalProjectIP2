#ifndef RECOMMENDATION_H
#define RECOMMENDATION_H

#include "graph.h"

typedef struct SplitOption {
    int cp1_id;
    int cp1_group_size;
    int cp2_id;
    int cp2_group_size;
    double combined_score;
} SplitOption;

typedef struct Recommendation {
    int is_split;               // 0 = Full group together, 1 = Split group
    int next_checkpoint_id;      // Used if is_split == 0
    int selected_path_index;     // Used if is_split == 0
    SplitOption split_info;      // Used if is_split == 1
    double score;
} Recommendation;

// Calculates fitness score for a path based on distance, group size, and availability
double calculate_path_score(Graph* g, Path path, int group_size, int round_idx);

// Recommends the next checkpoint, taking into account round/time slot and capacity
Recommendation recommend_next_checkpoint(Graph* g, int current_checkpoint, int group_size, int round_idx);

// Backward compatible recommendation (uses overall seats or first round)
Recommendation recommend_next_checkpoint_default(Graph* g, int current_checkpoint, int group_size);

// Recommends an interim checkpoint to visit in current_round before returning to desired_target in a later round
Recommendation recommend_interim_checkpoint(Graph* g, int current_cp, int desired_target, int group_size, int current_round);

#define MAX_ITINERARY_SLOTS 12
#define MAX_ALTERNATIVE_ITINERARIES 5

typedef struct ItinerarySchedule {
    int assigned_checkpoint_id[MAX_ITINERARY_SLOTS]; // Checkpoint ID per activity slot (-1 if free)
    int slot_round_idx[MAX_ITINERARY_SLOTS];         // Mapped round index (0-based) in Graph
    double spearman_penalty;                         // Sum of |desired_rank - assigned_slot|
    int is_valid;                                    // 1 if all k checkpoints successfully assigned
} ItinerarySchedule;

// Solve Linear Sum Assignment Problem (Hungarian Algorithm O(k^3)) for global optimal itinerary
ItinerarySchedule hungarian_optimize_itinerary(
    int k,
    const int* requested_checkpoints,
    int group_size,
    int num_slots,
    const int* activity_round_indices,
    const int capacity_matrix[MAX_ITINERARY_SLOTS][MAX_ITINERARY_SLOTS]
);

// Find Top-M valid itineraries using DFS with Backtracking and Branch-and-Bound Pruning
int dfs_top_itineraries(
    int k,
    const int* requested_checkpoints,
    int group_size,
    int num_slots,
    const int* activity_round_indices,
    const int capacity_matrix[MAX_ITINERARY_SLOTS][MAX_ITINERARY_SLOTS],
    int max_options,
    ItinerarySchedule options[MAX_ALTERNATIVE_ITINERARIES]
);

// Convenience RAM-first helper: loads capacity slice into memory once, runs Hungarian and DFS
int optimize_itineraries_in_ram(
    Graph* g,
    int k,
    const int* requested_checkpoints,
    int group_size,
    ItinerarySchedule* optimal_schedule,
    ItinerarySchedule alternatives[MAX_ALTERNATIVE_ITINERARIES],
    int* num_alternatives
);

// Uses Hungarian optimization matching to suggest the optimal available alternative checkpoint for round_idx, excluding already visited checkpoints
int hungarian_suggest_alternative(Graph* g, int current_loc, int round_idx, int rejected_target, int group_size, const int* excluded_checkpoints, int num_excluded);

typedef struct OptimalTour {
    int ordered_checkpoints[MAX_CHECKPOINTS];
    int count;
    double total_distance;
    Path leg_paths[MAX_CHECKPOINTS]; // leg i is path from ordered_checkpoints[i-1] (or start_node) to ordered_checkpoints[i]
} OptimalTour;

// Checks if a checkpoint has at least one activity round with available seats >= group_size
int checkpoint_has_available_round(Graph* g, int cp_id, int group_size);

// Checks if the exact requested order is directly feasible in natural activity slots
int is_exact_order_feasible(Graph* g, int k, const int* requested_checkpoints, int group_size);

// Finds optimal visiting order and shortest walking path across chosen checkpoints (without capacity restrictions)
OptimalTour find_optimal_tour(Graph* g, int start_node, const int* target_checkpoints, int count);

#endif