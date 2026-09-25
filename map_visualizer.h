#ifndef MAP_VISUALIZER_H
#define MAP_VISUALIZER_H

#include "graph.h"

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Returns the floor level (1, 2, or 3) for a given node name and room number.
 */
int get_node_floor_level(const char* name, int room_num);

/*
 * Returns the canonical identifier string for a node (e.g. "4stair", "343", "entry", etc.)
 */
const char* get_canonical_node_code(const char* name, int room_num);

/*
 * Displays the static ASCII floor map of a specific floor (1, 2, or 3).
 */
void display_floor_map(int floor_num);

/*
 * Displays the static ASCII floor map of all 3 floors.
 */
void display_all_floor_maps(void);

/*
 * Visualizes a walking path across all floors traversed.
 * Uses '*' to mark walking steps on that floor.
 * Uses '+' when transitioning UP a floor (via stairs/lifts/atrium).
 * Uses '-' when transitioning DOWN a floor (via stairs/lifts/entry).
 */
void visualize_path(Graph* g, const Path* path);

/*
 * Visualizes a walking path specifically on the given floor (1, 2, or 3).
 */
void visualize_floor_path(Graph* g, const Path* path, int floor_num);

#ifdef __cplusplus
}
#endif

#endif /* MAP_VISUALIZER_H */
