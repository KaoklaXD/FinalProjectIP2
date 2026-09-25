#ifndef STORAGE_H
#define STORAGE_H

#include "graph.h"

// Loads checkpoints from file into Graph. Returns number of checkpoints loaded, or -1 on error.
int load_checkpoints(const char* filename, Graph* g);

// Saves current checkpoints and their round availability to file. Returns 1 on success, 0 on error.
int save_checkpoints(const char* filename, Graph* g);

// Loads hallway / distance connections from file into Graph. Returns number of edges added, or -1 on error.
int load_distances(const char* filename, Graph* g);

// Looks up a checkpoint index by name or room number string. Returns index or -1 if not found.
int find_checkpoint_by_name(Graph* g, const char* name);

// Registers or finds a node by name in the graph, adding as waypoint if needed.
int get_or_create_node(Graph* g, const char* name);

// Checks whether a room qualifies as a valid checkpoint (room with number or 'audi')
int is_valid_checkpoint_room(const char* name, int room_num);

// Returns the list of round indices (0-indexed) that are valid activity rounds (ignoring lunch break)
int get_activity_rounds(Graph* g, int* round_indices, int max_out);

#endif
