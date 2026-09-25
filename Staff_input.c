#include <stdio.h>
#include <string.h>
#include <stdlib.h> 
#include <errno.h>
#include "graph.h"
#include "storage.h"

static int parse_int(const char *str, int *out_val) {
    if (!str || *str == '\0') return 0;
    char *endptr;
    errno = 0;
    long val = strtol(str, &endptr, 10);
    if (str == endptr || *endptr != '\0' || errno == ERANGE) {
        return 0;
    }
    *out_val = (int)val;
    return 1;
}

static int read_time_hhmm(const char *prompt, double *decimal_time) {
    char time_str[64];
    while (1) {
        printf("%s", prompt);
        if (scanf("%63s", time_str) != 1) return 0;

        char *colon = strchr(time_str, ':');
        if (!colon) {
            printf("[Error] Format must be HH:MM (e.g., 09:30). Please try again.\n");
            continue;
        }

        *colon = '\0';
        char *hour_str = time_str;
        char *min_str = colon + 1;

        int h = -1, m = -1;
        if (!parse_int(hour_str, &h) || !parse_int(min_str, &m)) {
            printf("[Error] Non-numeric time entered. Please try again.\n");
            continue;
        }

        if (h < 0 || h > 23 || m < 0 || m > 59) {
            printf("[Error] Invalid time range (Hour 0-23, Minute 0-59). Please try again.\n");
            continue;
        }

        *decimal_time = (double)h + ((double)m / 60.0);
        return 1;
    }
}

static void show_checkpoint_room_mapping(const char *filename, int *next_suggested_id) {
    Graph* g = create_graph(0);
    int count = load_checkpoints(filename, g);
    int max_id = -1;

    printf("\n========================================================================\n");
    printf("              CURRENT CHECKPOINT NUMBERS & ASSIGNED ROOMS               \n");
    printf("========================================================================\n");
    if (count <= 0) {
        printf("  No checkpoints registered yet.\n");
        *next_suggested_id = 0;
    } else {
        printf("  Checkpoint Number  |  Room Number  |  Checkpoint Name\n");
        printf(" --------------------+---------------+----------------------------------\n");
        for (int i = 0; i < g->num_checkpoints; i++) {
            if (strlen(g->nodes[i].name) > 0) {
                if (g->nodes[i].room_num > 0) {
                    printf("   Checkpoint %-6d |  Room %-7d |  %s\n",
                           g->nodes[i].id, g->nodes[i].room_num, g->nodes[i].name);
                } else {
                    printf("   Checkpoint %-6d |  Room %-7s |  %s\n",
                           g->nodes[i].id, g->nodes[i].name, g->nodes[i].name);
                }
                if (g->nodes[i].id > max_id) {
                    max_id = g->nodes[i].id;
                }
            }
        }
        *next_suggested_id = max_id + 1;
        printf(" --------------------+---------------+----------------------------------\n");
        printf("  >>> Next suggested Checkpoint Number: %d\n", *next_suggested_id);
    }
    printf("========================================================================\n\n");
    free_graph(g);
}

int add_new_checkpoint(const char *filename) {
    int id, round_num, max_seat, roomnum;
    char name[64];
    int next_id = 0;

    // Display existing checkpoint numbers and their assigned rooms
    show_checkpoint_room_mapping(filename, &next_id);

    printf("=======================================\n");
    printf("       STAFF CHECKPOINT ENTRY         \n");
    printf("=======================================\n");
    printf("Enter Checkpoint Number/ID (Suggested: %d, or -1 to cancel): ", next_id);
    if (scanf("%d", &id) != 1 || id < 0) {
        printf("Operation cancelled.\n");
        return 0;
    }

    printf("Enter Checkpoint Name (e.g., PhysicsLab or audi1): ");
    scanf("%63s", name);

    printf("Enter Room Number (e.g., 122, 212, or 0 for auditorium): ");
    if (scanf("%d", &roomnum) != 1) roomnum = 0;

    if (!is_valid_checkpoint_room(name, roomnum)) {
        printf("\n[ERROR] Invalid Checkpoint Room!\n");
        printf("Policy Rule: Only rooms with a room number (e.g., 122, 212) or auditoriums ('audi1', 'audi2') can be used as checkpoints.\n");
        printf("Hallways, stairs, lifts, and facilities cannot be checkpoints.\n");
        return 0;
    }

    printf("\n--> Checkpoint Number %d is assigned to Room %d (%s)\n", id, roomnum, name);

    printf("Enter number of rounds (1-%d): ", MAX_ROUNDS);
    if (scanf("%d", &round_num) != 1 || round_num <= 0 || round_num > MAX_ROUNDS) {
        printf("[Error] Invalid round count.\n");
        return 0;
    }

    printf("Enter maximum seats: ");
    if (scanf("%d", &max_seat) != 1 || max_seat <= 0) {
        printf("[Error] Invalid seat capacity.\n");
        return 0;
    }

    printf("\nRound Schedule Option:\n");
    printf(" [1] Apply Synchronized Parallel Schedule (7 rounds, equal start/final, Round 4 Lunch Break)\n");
    printf(" [2] Manual Custom Entry\n");
    printf("Select (1 or 2): ");
    int sched_choice = 1;
    if (scanf("%d", &sched_choice) != 1) sched_choice = 1;

    double starttime[MAX_ROUNDS];
    double endtime[MAX_ROUNDS];
    int is_lunch[MAX_ROUNDS] = {0};

    if (sched_choice == 1) {
        round_num = 7;
        double default_starts[7] = {9.00, 10.00, 11.00, 12.00, 13.00, 14.00, 15.00};
        double default_ends[7]   = {10.00, 11.00, 12.00, 13.00, 14.00, 15.00, 16.00};
        for (int i = 0; i < 7; i++) {
            starttime[i] = default_starts[i];
            endtime[i] = default_ends[i];
            if (i == 3) is_lunch[i] = 1; // Round 4 is Lunch Break
        }
        printf("\n[Applied] Synchronized Parallel Schedule applied (6 activity rounds + Round 4 Lunch Break).\n");
    } else {
        printf("Enter number of rounds (1-%d): ", MAX_ROUNDS);
        if (scanf("%d", &round_num) != 1 || round_num <= 0 || round_num > MAX_ROUNDS) {
            printf("[Error] Invalid round count.\n");
            return 0;
        }

        for (int i = 0; i < round_num; i++) {
            char prompt[128];
            printf("\n--- Round %d Configuration ---\n", i + 1);
            snprintf(prompt, sizeof(prompt), "Enter Round %d START time (HH:MM): ", i + 1);
            read_time_hhmm(prompt, &starttime[i]);

            snprintf(prompt, sizeof(prompt), "Enter Round %d END time   (HH:MM): ", i + 1);
            read_time_hhmm(prompt, &endtime[i]);

            printf("Is this round a Lunch Break / empty section? (1 = Yes, 0 = No): ");
            int lb = 0;
            if (scanf("%d", &lb) == 1 && lb == 1) {
                is_lunch[i] = 1;
            }
        }
    }

    // Append to checkpoints.txt
    FILE *fp = fopen(filename, "a");
    if (fp == NULL) {
        printf("[Error] Unable to open '%s' for writing.\n", filename);
        return 0;
    }

    fprintf(fp, "CHECKPOINT %d %s %d %d %d\n", id, name, round_num, max_seat, roomnum);
    for (int i = 0; i < round_num; i++) {
        if (is_lunch[i]) {
            fprintf(fp, "ROUND %d %.2f %.2f 0 LUNCH\n", i + 1, starttime[i], endtime[i]);
        } else {
            fprintf(fp, "ROUND %d %.2f %.2f %d\n", i + 1, starttime[i], endtime[i], max_seat);
        }
    }
    fprintf(fp, "END_CHECKPOINT\n\n");
    fclose(fp);

    printf("\n[SUCCESS] Checkpoint Number %d -> Room %d (%s) saved to '%s'!\n", id, roomnum, name, filename);
    return 1;
}

int main() {
    int choice = 0;
    while (1) {
        printf("\n=======================================\n");
        printf("   KVIS OPEN HOUSE: STAFF CONTROL PANEL\n");
        printf("=======================================\n");
        printf(" [1] Add New Checkpoint & Rounds\n");
        printf(" [2] Exit\n");
        printf("Enter selection: ");

        if (scanf("%d", &choice) != 1 || choice == 2) {
            printf("\nExiting Staff Control Panel. Goodbye!\n");
            break;
        }

        if (choice == 1) {
            add_new_checkpoint("checkpoints.txt");
        } else {
            printf("Invalid selection! Please enter 1 or 2.\n");
        }
    }
    return 0;
}
