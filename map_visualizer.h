#ifndef MAP_VISUALIZER_H
#define MAP_VISUALIZER_H

#include "graph.h"

#ifdef __cplusplus
extern "C" {
#endif

int get_node_floor_level(const char* name, int room_num);

const char* get_canonical_node_code(const char* name, int room_num);

void display_floor_map(int floor_num);

void display_all_floor_maps(void);

void visualize_path(Graph* g, const Path* path);

void visualize_floor_path(Graph* g, const Path* path, int floor_num);

#ifdef __cplusplus
}
#endif

#endif
