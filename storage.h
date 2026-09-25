#ifndef STORAGE_H
#define STORAGE_H

#include "graph.h"

int load_checkpoints(const char* filename, Graph* g);

int save_checkpoints(const char* filename, Graph* g);

int load_distances(const char* filename, Graph* g);

int find_checkpoint_by_name(Graph* g, const char* name);

int get_or_create_node(Graph* g, const char* name);

int is_valid_checkpoint_room(const char* name, int room_num);

int get_activity_rounds(Graph* g, int* round_indices, int max_out);

int save_itinerary_booking(const char* filename, Graph* g, const char* group_name, const char* member_name, int group_size, const int* assigned_checkpoints, int total_rounds);

int save_tour_booking(const char* filename, Graph* g, const char* group_name, const char* member_name, int group_size, int start_node, const int* ordered_checkpoints, int count, double total_distance);

int display_visitor_bookings(const char* filename);

#endif
