#include <stdio.h>
#include <string.h>
#include <stdlib.h> 
#include <errno.h>

void strToInt(char str[50], int *num, int *i) {
    char *endptr;
    long val = strtol(str, &endptr, 10);

    if (str == endptr) {
        printf("Invalid format\n");
        (*i)--;
    } 
    else if (*endptr != '\0') {
        printf("Error: Invalid numeric string\n");
        (*i)--;
    }
    else if (errno == ERANGE) {
        printf("Number out of ERANGE\n");
        (*i)--;
    }
    else {
        *num = (int)val;
    }
}

int save_staff_checkpoint_input(const char *filename) {
    int id, round_num, max_seat,roomnum;
    char name[50];

    printf("=======================================\n");
    printf("       STAFF CHECKPOINT ENTRY         \n");
    printf("=======================================\n");
    printf("Enter checkpoint ID (or -1 to cancel): ");
    if (scanf("%d", &id) != 1 || id == -1) {
        printf("Operation cancelled.\n");
        return 0;
    }

    printf("Enter checkpoint name: ");
    scanf("%49s", name);

    printf("Enter Roomnum: ");
    scanf("%d", &roomnum);

    printf("Enter checkpoint round number: ");
    scanf("%d", &round_num);

    printf("Enter max_seat: ");
    scanf("%d", &max_seat);

    double *starttime = (double *)malloc(round_num * sizeof(double));
    double *endtime = (double *)malloc(round_num * sizeof(double));

    if (!starttime || !endtime) {
        printf("Memory allocation error.\n");
        free(starttime);
        free(endtime);
        return 0;
    }

    for (int i = 0; i < round_num; i++) {
        int valid = 1;
        char time[50];
        char hour[50], minute[50];

        // --- Start Time ---
        printf("\nEnter round %d START time in HH:MM: ", i + 1);
        scanf("%49s", time);

        for (int j = 0; j < (int)strlen(time); j++) {
            if (!((time[j] >= '0' && time[j] <= '9') || (time[j] == ':'))) {
                printf("Invalid format\n");
                valid = 0;
                i--;
                break;
            }
        }
        if (!valid) continue;

        char *token = strtok(time, ":");
        if (token != NULL) strcpy(hour, token);
        token = strtok(NULL, ":");
        if (token != NULL) strcpy(minute, token);

        int h = -1, m = -1;
        strToInt(hour, &h, &i);
        strToInt(minute, &m, &i);

        if (h > 23 || h < 0 || m > 59 || m < 0) {
            printf("Invalid time! Please enter HH:MM in valid range.\n");
            i--;
            continue;
        }

        starttime[i] = h + (m / 60.0);

        // --- End Time ---
        valid = 1;
        printf("Enter round %d END time in HH:MM: ", i + 1);
        scanf("%49s", time);

        for (int j = 0; j < (int)strlen(time); j++) {
            if (!((time[j] >= '0' && time[j] <= '9') || (time[j] == ':'))) {
                printf("Invalid format\n");
                valid = 0;
                i--;
                break;
            }
        }
        if (!valid) continue;

        token = strtok(time, ":");
        if (token != NULL) strcpy(hour, token);
        token = strtok(NULL, ":");
        if (token != NULL) strcpy(minute, token);

        strToInt(hour, &h, &i);
        strToInt(minute, &m, &i);

        if (h > 23 || h < 0 || m > 59 || m < 0) {
            printf("Invalid time! Please enter HH:MM in valid range.\n");
            i--;
            continue;
        }

        endtime[i] = h + (m / 60.0);
    }

    // --- File Writing Engine ---
    FILE *fp = fopen(filename, "a");
    if (fp == NULL) {
        printf("Error opening file '%s' for writing!\n", filename);
        free(starttime);
        free(endtime);
        return 0;
    }

    fprintf(fp, "CHECKPOINT %d %s %d %d %d\n", id, name, round_num, max_seat,roomnum);
    for (int i = 0; i < round_num; i++) {
        fprintf(fp, "ROUND %d %.2f %.2f\n", i + 1, starttime[i], endtime[i]);
    }
    fprintf(fp, "END_CHECKPOINT\n\n");

    fclose(fp);
    free(starttime);
    free(endtime);

    printf("\n[SUCCESS] Saved checkpoint ID %d to '%s'\n\n", id, filename);
    return 1;
}

int main() {
    save_staff_checkpoint_input("checkpoints.txt");
    return 0;
}