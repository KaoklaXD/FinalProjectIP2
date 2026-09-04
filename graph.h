#ifndef GRAPH_H
#define GRAPH_H

#define MAX_CHECKPOINTS 50
#define INF 1e9

typedef struct Edge {
    int to;
    double distance;
    struct Edge* next;
} Edge;

typedef struct Checkpoint {
    int id;
    char name[50];
    char desc[200];
    int available_seats;
    int max_seats;
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
void eppstein_k_shortest_paths(Graph* g, int start, int target, int k, Path results[], int* result_count);


#endif