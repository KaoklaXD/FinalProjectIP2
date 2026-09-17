#ifndef GRAPH_H
#define GRAPH_H

#define MAX_CHECKPOINTS 100
#define MAX_ROUNDS 10
#define INF 1e9

typedef struct Edge {
    int to;
    double distance;
    int disabled;
    struct Edge* next;
} Edge;

typedef struct Round {
    int round_id;
    double start_time;    // e.g. 9.50 = 09:30
    double end_time;      // e.g. 12.50 = 12:30
    int max_seats;
    int available_seats;
} Round;

typedef struct Checkpoint {
    int id;
    char name[64];
    char desc[200];
    int room_num;
    int num_rounds;
    Round rounds[MAX_ROUNDS];
    int available_seats;  // Default / overall available seats
    int max_seats;        // Default / overall maximum seats
    Edge* head;
} Checkpoint;

typedef struct Graph {
    int num_checkpoints;
    Checkpoint nodes[MAX_CHECKPOINTS];
} Graph;

typedef struct Path {
    int nodes[MAX_CHECKPOINTS];
    int node_count;
    double total_distance;
} Path;

// Function declarations
Graph* create_graph(int num_checkpoints);
void add_edge(Graph* g, int u, int v, double distance);
void add_bi_edge(Graph* g, int u, int v, double distance);
void free_graph(Graph* g);

int dijkstra_shortest_path(Graph* g, int start, int target, const int* blocked_nodes, int num_blocked, Path* result);
void k_shortest_paths(Graph* g, int start, int target, int k, Path results[], int* result_count);
void eppstein_k_shortest_paths(Graph* g, int start, int target, int k, Path results[], int* result_count);

#endif