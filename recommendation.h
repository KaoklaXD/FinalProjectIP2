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

#endif