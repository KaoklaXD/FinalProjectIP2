#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "storage.h"

static char* trim(char* str) {
    if (!str) return NULL;
    while (isspace((unsigned char)*str)) str++;
    if (*str == 0) return str;
    char* end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end)) end--;
    end[1] = '\0';
    return str;
}

int find_checkpoint_by_name(Graph* g, const char* name) {
    if (!g || !name) return -1;
    for (int i = 0; i < g->num_checkpoints; i++) {
        // 1. Direct case-insensitive match
        if (strcasecmp(g->nodes[i].name, name) == 0) {
            return i;
        }

        // 2. Room number match
        if (g->nodes[i].room_num > 0) {
            char room_str[16];
            snprintf(room_str, sizeof(room_str), "%d", g->nodes[i].room_num);
            if (strcmp(room_str, name) == 0) {
                return i;
            }
            // Prefix room match (e.g., token "112(dream)" matches room 112)
            if (strncmp(name, room_str, strlen(room_str)) == 0) {
                return i;
            }
        }

        // 3. Substring match (e.g., "dream" inside "112(dream)" matches "DreamLab")
        if (strlen(g->nodes[i].name) > 0 && strlen(name) >= 3) {
            if (strcasestr(name, g->nodes[i].name) != NULL ||
                strcasestr(g->nodes[i].name, name) != NULL) {
                return i;
            }
        }
    }
    return -1;
}

int get_or_create_node(Graph* g, const char* name) {
    if (!g || !name) return -1;
    char clean_name[64];
    strncpy(clean_name, name, sizeof(clean_name) - 1);
    clean_name[sizeof(clean_name) - 1] = '\0';
    char* trimmed = trim(clean_name);

    if (strlen(trimmed) == 0) return -1;

    int existing = find_checkpoint_by_name(g, trimmed);
    if (existing != -1) return existing;

    // Check if name is pure integer matching an ID
    char* endptr;
    long val = strtol(trimmed, &endptr, 10);
    if (*endptr == '\0' && val >= 0 && val < g->num_checkpoints) {
        return (int)val;
    }

    if (g->num_checkpoints < MAX_CHECKPOINTS) {
        int idx = g->num_checkpoints++;
        g->nodes[idx].id = idx;
        strncpy(g->nodes[idx].name, trimmed, sizeof(g->nodes[idx].name) - 1);
        g->nodes[idx].name[sizeof(g->nodes[idx].name) - 1] = '\0';
        g->nodes[idx].available_seats = 50;
        g->nodes[idx].max_seats = 50;
        g->nodes[idx].num_rounds = 0;
        g->nodes[idx].room_num = 0;
        g->nodes[idx].head = NULL;
        return idx;
    }
    return -1;
}

int load_checkpoints(const char* filename, Graph* g) {
    FILE* fp = fopen(filename, "r");
    if (!fp) {
        return -1;
    }

    char line[256];
    int current_id = -1;
    int count = 0;

    while (fgets(line, sizeof(line), fp)) {
        char* trimmed = trim(line);
        if (strlen(trimmed) == 0) continue;

        if (strncmp(trimmed, "CHECKPOINT", 10) == 0) {
            int id = -1, round_num = 0, max_seat = 0, roomnum = 0;
            char name[64] = {0};
            
            int matched = sscanf(trimmed, "CHECKPOINT %d %63s %d %d %d", &id, name, &round_num, &max_seat, &roomnum);
            if (matched >= 4 && id >= 0 && id < MAX_CHECKPOINTS) {
                current_id = id;
                if (id >= g->num_checkpoints) {
                    g->num_checkpoints = id + 1;
                }
                g->nodes[id].id = id;
                strncpy(g->nodes[id].name, name, sizeof(g->nodes[id].name) - 1);
                g->nodes[id].num_rounds = round_num;
                g->nodes[id].max_seats = max_seat;
                g->nodes[id].available_seats = max_seat;
                g->nodes[id].room_num = roomnum;
                count++;
            }
        } else if (strncmp(trimmed, "ROUND", 5) == 0 && current_id >= 0) {
            int round_idx = 0;
            double start_t = 0.0, end_t = 0.0;
            int avail = -1;

            int matched = sscanf(trimmed, "ROUND %d %lf %lf %d", &round_idx, &start_t, &end_t, &avail);
            if (matched >= 3 && round_idx >= 1 && round_idx <= MAX_ROUNDS) {
                int r = round_idx - 1;
                g->nodes[current_id].rounds[r].round_id = round_idx;
                g->nodes[current_id].rounds[r].start_time = start_t;
                g->nodes[current_id].rounds[r].end_time = end_t;
                g->nodes[current_id].rounds[r].max_seats = g->nodes[current_id].max_seats;
                g->nodes[current_id].rounds[r].available_seats = (avail >= 0) ? avail : g->nodes[current_id].max_seats;
                g->nodes[current_id].available_seats = g->nodes[current_id].rounds[0].available_seats;
            }
        } else if (strncmp(trimmed, "END_CHECKPOINT", 14) == 0) {
            current_id = -1;
        }
    }

    fclose(fp);
    return count;
}

int save_checkpoints(const char* filename, Graph* g) {
    if (!g) return 0;
    FILE* fp = fopen(filename, "w");
    if (!fp) return 0;

    for (int i = 0; i < g->num_checkpoints; i++) {
        if (strlen(g->nodes[i].name) == 0 || g->nodes[i].num_rounds <= 0) {
            continue;
        }
        fprintf(fp, "CHECKPOINT %d %s %d %d %d\n",
                g->nodes[i].id,
                g->nodes[i].name,
                g->nodes[i].num_rounds,
                g->nodes[i].max_seats,
                g->nodes[i].room_num);

        for (int r = 0; r < g->nodes[i].num_rounds; r++) {
            fprintf(fp, "ROUND %d %.2f %.2f %d\n",
                    g->nodes[i].rounds[r].round_id,
                    g->nodes[i].rounds[r].start_time,
                    g->nodes[i].rounds[r].end_time,
                    g->nodes[i].rounds[r].available_seats);
        }
        fprintf(fp, "END_CHECKPOINT\n\n");
    }

    fclose(fp);
    return 1;
}

int load_distances(const char* filename, Graph* g) {
    FILE* fp = fopen(filename, "r");
    if (!fp) return -1;

    char line[512];
    int edges_added = 0;

    while (fgets(line, sizeof(line), fp)) {
        char* trimmed = trim(line);
        if (strlen(trimmed) == 0) continue;

        // Skip non-corridor lines that have no '-' and are not EDGE lines
        if (strchr(trimmed, '-') == NULL && strncmp(trimmed, "EDGE", 4) != 0) {
            continue;
        }

        // Direct EDGE definition: EDGE u v distance
        if (strncmp(trimmed, "EDGE", 4) == 0) {
            char u_str[64], v_str[64];
            double dist = 10.0;
            if (sscanf(trimmed, "EDGE %63s %63s %lf", u_str, v_str, &dist) >= 2) {
                int u = get_or_create_node(g, u_str);
                int v = get_or_create_node(g, v_str);
                if (u >= 0 && v >= 0) {
                    add_bi_edge(g, u, v, dist);
                    edges_added++;
                }
            }
            continue;
        }

        // Hyphen chain corridor: entry-audi2-212-213-1stair
        char* token = strtok(trimmed, "-");
        int prev_node = -1;
        while (token != NULL) {
            char* t_trim = trim(token);
            if (strlen(t_trim) > 0) {
                int curr_node = get_or_create_node(g, t_trim);
                if (prev_node != -1 && curr_node != -1) {
                    add_bi_edge(g, prev_node, curr_node, 10.0); // Exactly 10m per adjacent hallway segment
                    edges_added++;
                }
                prev_node = curr_node;
            }
            token = strtok(NULL, "-");
        }
    }

    fclose(fp);
    return edges_added;
}

int is_valid_checkpoint_room(const char* name, int room_num) {
    if (!name) return 0;
    
    // Explicit exclusions (waypoints, stairs, lifts, facilities, hallways)
    if (strcasestr(name, "stair") != NULL) return 0;
    if (strcasestr(name, "lift") != NULL) return 0;
    if (strcasestr(name, "wing") != NULL) return 0;
    if (strcasecmp(name, "entry") == 0) return 0;
    if (strcasecmp(name, "arc") == 0) return 0;
    if (strcasecmp(name, "ground") == 0) return 0;
    if (strcasecmp(name, "pingpong") == 0) return 0;
    if (strcasecmp(name, "atrium") == 0) return 0;
    if (strcasecmp(name, "taobin") == 0) return 0;
    if (strcasecmp(name, "bathroom") == 0) return 0;
    if (strcasecmp(name, "canteen") == 0) return 0;

    // Rule: Must contain "audi" OR have a room number (digits)
    if (strcasestr(name, "audi") != NULL) {
        return 1;
    }

    if (room_num > 0) {
        return 1;
    }

    for (int i = 0; name[i] != '\0'; i++) {
        if (isdigit((unsigned char)name[i])) {
            return 1;
        }
    }

    return 0;
}
