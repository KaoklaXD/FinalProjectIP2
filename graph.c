#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include "graph.h"

Graph* create_graph(int num_checkpoints) {
    if (num_checkpoints > MAX_CHECKPOINTS) {
        num_checkpoints = MAX_CHECKPOINTS;
    }
    Graph* g = (Graph*)malloc(sizeof(Graph));
    if (!g) return NULL;

    g->num_checkpoints = num_checkpoints;
    for (int i = 0; i < MAX_CHECKPOINTS; i++) {
        g->nodes[i].id = i;
        g->nodes[i].name[0] = '\0';
        g->nodes[i].desc[0] = '\0';
        g->nodes[i].room_num = 0;
        g->nodes[i].num_rounds = 0;
        g->nodes[i].available_seats = 0;
        g->nodes[i].max_seats = 0;
        g->nodes[i].head = NULL;
        for (int r = 0; r < MAX_ROUNDS; r++) {
            g->nodes[i].rounds[r].round_id = r + 1;
            g->nodes[i].rounds[r].start_time = 0.0;
            g->nodes[i].rounds[r].end_time = 0.0;
            g->nodes[i].rounds[r].max_seats = 0;
            g->nodes[i].rounds[r].available_seats = 0;
        }
    }
    return g;
}

void add_edge(Graph* g, int u, int v, double distance) {
    if (!g || u < 0 || u >= MAX_CHECKPOINTS || v < 0 || v >= MAX_CHECKPOINTS) {
        return;
    }
    Edge* edge = (Edge*)malloc(sizeof(Edge));
    if (!edge) return;
    edge->to = v;
    edge->distance = distance;
    edge->disabled = 0;
    edge->next = g->nodes[u].head;
    g->nodes[u].head = edge;
}

void add_bi_edge(Graph* g, int u, int v, double distance) {
    add_edge(g, u, v, distance);
    add_edge(g, v, u, distance);
}

void free_graph(Graph* g) {
    if (!g) return;
    for (int i = 0; i < MAX_CHECKPOINTS; i++) {
        Edge* curr = g->nodes[i].head;
        while (curr) {
            Edge* tmp = curr;
            curr = curr->next;
            free(tmp);
        }
        g->nodes[i].head = NULL;
    }
    free(g);
}

int dijkstra_shortest_path(Graph* g, int start, int target, const int* blocked_nodes, int num_blocked, Path* result) {
    if (!g || start < 0 || start >= g->num_checkpoints || target < 0 || target >= g->num_checkpoints) {
        return 0;
    }

    double dist[MAX_CHECKPOINTS];
    int parent[MAX_CHECKPOINTS];
    bool visited[MAX_CHECKPOINTS];

    for (int i = 0; i < g->num_checkpoints; i++) {
        dist[i] = INF;
        parent[i] = -1;
        visited[i] = false;
    }

    for (int b = 0; b < num_blocked; b++) {
        int node = blocked_nodes[b];
        if (node >= 0 && node < g->num_checkpoints) {
            visited[node] = true;
        }
    }

    dist[start] = 0;
    visited[start] = false;

    for (int count = 0; count < g->num_checkpoints; count++) {
        double min_dist = INF;
        int u = -1;

        for (int v = 0; v < g->num_checkpoints; v++) {
            if (!visited[v] && dist[v] < min_dist) {
                min_dist = dist[v];
                u = v;
            }
        }

        if (u == -1 || u == target) break;
        visited[u] = true;

        for (Edge* curr = g->nodes[u].head; curr != NULL; curr = curr->next) {
            if (curr->disabled) continue;
            int v = curr->to;
            if (!visited[v] && dist[u] + curr->distance < dist[v]) {
                dist[v] = dist[u] + curr->distance;
                parent[v] = u;
            }
        }
    }

    if (dist[target] >= INF / 2.0) {
        return 0;
    }

    int temp[MAX_CHECKPOINTS];
    int len = 0;
    int curr = target;
    while (curr != -1 && len < MAX_CHECKPOINTS) {
        temp[len++] = curr;
        if (curr == start) break;
        curr = parent[curr];
    }

    if (len == 0 || temp[len - 1] != start) {
        return 0;
    }

    result->node_count = len;
    result->total_distance = dist[target];
    for (int i = 0; i < len; i++) {
        result->nodes[i] = temp[len - 1 - i];
    }
    return 1;
}

static bool paths_are_equal(const Path* p1, const Path* p2) {
    if (p1->node_count != p2->node_count) return false;
    for (int i = 0; i < p1->node_count; i++) {
        if (p1->nodes[i] != p2->nodes[i]) return false;
    }
    return true;
}

void k_shortest_paths(Graph* g, int start, int target, int k, Path results[], int* result_count) {
    *result_count = 0;
    if (!g || k <= 0 || start < 0 || start >= g->num_checkpoints || target < 0 || target >= g->num_checkpoints) {
        return;
    }

    Path p0;
    if (!dijkstra_shortest_path(g, start, target, NULL, 0, &p0)) {
        return;
    }

    results[(*result_count)++] = p0;

    Path candidates[100];
    int candidate_count = 0;

    for (int k_idx = 1; k_idx < k; k_idx++) {
        const Path* prev_path = &results[k_idx - 1];

        for (int i = 0; i < prev_path->node_count - 1; i++) {
            int spur_node = prev_path->nodes[i];
            Path root_path;
            root_path.node_count = i + 1;
            root_path.total_distance = 0.0;
            for (int r = 0; r <= i; r++) {
                root_path.nodes[r] = prev_path->nodes[r];
            }

            for (int p = 0; p < *result_count; p++) {
                const Path* existing = &results[p];
                if (existing->node_count > i + 1) {
                    bool match = true;
                    for (int r = 0; r <= i; r++) {
                        if (existing->nodes[r] != root_path.nodes[r]) {
                            match = false;
                            break;
                        }
                    }
                    if (match) {
                        int u = existing->nodes[i];
                        int v = existing->nodes[i + 1];
                        for (Edge* e = g->nodes[u].head; e != NULL; e = e->next) {
                            if (e->to == v) {
                                e->disabled = 1;
                            }
                        }
                    }
                }
            }

            int blocked_nodes[MAX_CHECKPOINTS];
            int num_blocked = 0;
            for (int r = 0; r < i; r++) {
                blocked_nodes[num_blocked++] = root_path.nodes[r];
            }

            Path spur_path;
            if (dijkstra_shortest_path(g, spur_node, target, blocked_nodes, num_blocked, &spur_path)) {
                Path total_candidate;
                total_candidate.node_count = 0;
                total_candidate.total_distance = 0.0;

                for (int r = 0; r <= i; r++) {
                    total_candidate.nodes[total_candidate.node_count++] = root_path.nodes[r];
                }

                for (int s = 1; s < spur_path.node_count; s++) {
                    total_candidate.nodes[total_candidate.node_count++] = spur_path.nodes[s];
                }

                double dist_acc = 0.0;
                bool valid_dist = true;
                for (int s = 0; s < total_candidate.node_count - 1; s++) {
                    int u = total_candidate.nodes[s];
                    int v = total_candidate.nodes[s + 1];
                    double edge_d = INF;
                    for (Edge* e = g->nodes[u].head; e != NULL; e = e->next) {
                        if (e->to == v && e->distance < edge_d) {
                            edge_d = e->distance;
                        }
                    }
                    if (edge_d >= INF / 2.0) {
                        valid_dist = false;
                        break;
                    }
                    dist_acc += edge_d;
                }

                if (valid_dist) {
                    total_candidate.total_distance = dist_acc;

                    bool exists = false;
                    for (int p = 0; p < *result_count; p++) {
                        if (paths_are_equal(&total_candidate, &results[p])) {
                            exists = true;
                            break;
                        }
                    }
                    for (int c = 0; c < candidate_count; c++) {
                        if (paths_are_equal(&total_candidate, &candidates[c])) {
                            exists = true;
                            break;
                        }
                    }

                    if (!exists && candidate_count < 100) {
                        candidates[candidate_count++] = total_candidate;
                    }
                }
            }

            for (int n = 0; n < g->num_checkpoints; n++) {
                for (Edge* e = g->nodes[n].head; e != NULL; e = e->next) {
                    e->disabled = 0;
                }
            }
        }

        if (candidate_count == 0) {
            break;
        }

        int best_c = 0;
        double min_dist = candidates[0].total_distance;
        for (int c = 1; c < candidate_count; c++) {
            if (candidates[c].total_distance < min_dist) {
                min_dist = candidates[c].total_distance;
                best_c = c;
            }
        }

        results[(*result_count)++] = candidates[best_c];

        candidates[best_c] = candidates[candidate_count - 1];
        candidate_count--;
    }
}

void eppstein_k_shortest_paths(Graph* g, int start, int target, int k, Path results[], int* result_count) {
    k_shortest_paths(g, start, target, k, results, result_count);
}
