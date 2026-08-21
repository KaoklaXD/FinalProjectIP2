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

Recommendation recommend_next_checkpoint(Graph* g, int current_checkpoint, int group_size);

#endif