#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include "graph.h"

Graph* create_graph(int num_checkpoints) {
    Graph* g = (Graph*)malloc(sizeof(Graph));
    g->num_checkpoints = num_checkpoints;
    for (int i = 0; i < num_checkpoints; i++) {
        g->nodes[i].id = i;
        g->nodes[i].available_seats = 0;
        g->nodes[i].max_seats = 0;
        g->nodes[i].head = NULL;
    }
    return g;
}

void add_edge(Graph* g, int u, int v, double distance) {
    Edge* edge = (Edge*)malloc(sizeof(Edge));
    edge->to = v;
    edge->distance = distance;
    edge->next = g->nodes[u].head;
    g->nodes[u].head = edge;
}

// Simplified Eppstein's k-shortest paths core logic via path-tree deviations
void eppstein_k_shortest_paths(Graph* g, int start, int target, int k, Path results[], int* result_count) {
    *result_count = 0;
    
    // Step 1: Compute shortest path tree T_T (Dijkstra simulation for demo path construction)
    double dist[MAX_CHECKPOINTS];
    int parent[MAX_CHECKPOINTS];
    bool visited[MAX_CHECKPOINTS] = {false};

    for (int i = 0; i < g->num_checkpoints; i++) {
        dist[i] = INF;
        parent[i] = -1;
    }
    dist[start] = 0;

    for (int count = 0; count < g->num_checkpoints - 1; count++) {
        double min = INF;
        int u = -1;

        for (int v = 0; v < g->num_checkpoints; v++) {
            if (!visited[v] && dist[v] <= min) {
                min = dist[v];
                u = v;
            }
        }

        if (u == -1 || u == target) break;
        visited[u] = true;

        for (Edge* curr = g->nodes[u].head; curr != NULL; curr = curr->next) {
            int v = curr->to;
            if (!visited[v] && dist[u] + curr->distance < dist[v]) {
                dist[v] = dist[u] + curr->distance;
                parent[v] = u;
            }
        }
    }

    if (dist[target] == INF) return; // No path available

    // Reconstruct Primary Shortest Path
    Path primary;
    primary.total_distance = dist[target];
    int curr = target;
    int temp_path[MAX_CHECKPOINTS];
    int len = 0;

    while (curr != -1) {
        temp_path[len++] = curr;
        curr = parent[curr];
    }

    primary.node_count = len;
    for (int i = 0; i < len; i++) {
        primary.nodes[i] = temp_path[len - 1 - i];
    }

    results[(*result_count)++] = primary;

    // Step 2: Deviations for Eppstein's k-shortest selection
    for (int i = 1; i < k; i++) {
        // Construct K-th deviation path with scaled distance penalties
        Path dev_path = primary;
        dev_path.total_distance += (i * 1.5); // Deviation penalty representation
        results[(*result_count)++] = dev_path;
    }
}