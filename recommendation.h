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
    int is_split;
    int next_checkpoint_id;
    int selected_path_index;
    SplitOption split_info;
    double score;
} Recommendation;

double calculate_path_score(Graph* g, Path path, int group_size, int round_idx);

Recommendation recommend_next_checkpoint(Graph* g, int current_checkpoint, int group_size, int round_idx);

Recommendation recommend_next_checkpoint_default(Graph* g, int current_checkpoint, int group_size);

Recommendation recommend_interim_checkpoint(Graph* g, int current_cp, int desired_target, int group_size, int current_round);

#define MAX_ITINERARY_SLOTS 12
#define MAX_ALTERNATIVE_ITINERARIES 5

typedef struct ItinerarySchedule {
    int assigned_checkpoint_id[MAX_ITINERARY_SLOTS];
    int slot_round_idx[MAX_ITINERARY_SLOTS];
    double spearman_penalty;
    int is_valid;
} ItinerarySchedule;

ItinerarySchedule hungarian_optimize_itinerary(
    int k,
    const int* requested_checkpoints,
    int group_size,
    int num_slots,
    const int* activity_round_indices,
    const int capacity_matrix[MAX_ITINERARY_SLOTS][MAX_ITINERARY_SLOTS]
);

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

int optimize_itineraries_in_ram(
    Graph* g,
    int k,
    const int* requested_checkpoints,
    int group_size,
    ItinerarySchedule* optimal_schedule,
    ItinerarySchedule alternatives[MAX_ALTERNATIVE_ITINERARIES],
    int* num_alternatives
);

int hungarian_suggest_alternative(Graph* g, int current_loc, int round_idx, int rejected_target, int group_size, const int* excluded_checkpoints, int num_excluded);

typedef struct OptimalTour {
    int ordered_checkpoints[MAX_CHECKPOINTS];
    int count;
    double total_distance;
    Path leg_paths[MAX_CHECKPOINTS];
} OptimalTour;

int checkpoint_has_available_round(Graph* g, int cp_id, int group_size);

int is_exact_order_feasible(Graph* g, int k, const int* requested_checkpoints, int group_size);

OptimalTour find_optimal_tour(Graph* g, int start_node, const int* target_checkpoints, int count);

#endif
