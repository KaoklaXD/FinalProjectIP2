#include <stdio.h>

#include <string.h>
#include <stdlib.h> 
#include <errno.h>

void strToInt(char str[50], int *num, int *i) {
    char *endptr;
    long val = strtol(str, &endptr, 10);

    if (str == endptr) {
        printf("Invalid format \n");
        *i--;
    } 
    else if (*endptr != '\0') {
        printf("WTF");
        *i--;
    }
    else if (errno == ERANGE) {
        printf("Number out of ERANGE \n");
        *i--;
    }
    else {
        int result = (int)val; 
        *num = result;
    }
}
int main() {
    while (1==1) {
        int id,round_num,max_seat;
        char name[50];
        printf("PLease Enter Information \n");
        printf("Enter checkpoint id: ");
        scanf("%d",&id);

        printf("Enter checkpoint name: ");
        scanf("%s",&name);

        printf("Enter checkpoint round number: ");
        scanf("%d",&round_num);

        printf("Enter max_seat: ");
        scanf("%d",&max_seat);
        double starttime[round_num];
        double endtime[round_num];

        for (int i = 0; i < round_num; i++) {

        int valid = 1;
        char time[50];
        char hour[50], minute[50];
    
        printf("Enter round %d start time in HH:MM \n", i + 1);
        scanf("%s", time);
    
        for (int j = 0; j < strlen(time); j++) {
            if (!((time[j] >= '0' && time[j] <= '9') || (time[j] == ':'))) {
                printf("Invalid format \n");
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
    
        int h, m;
        strToInt(hour, &h, &i);
        strToInt(minute, &m, &i);

        if (h > 23 || h < 0 || m > 59 || m < 0) {
            printf("The time you entered is invalid, please enter in a valid HH:MM format\n");
            i--;
            continue;
        }


        starttime[i] = h + (m / 60.0);


        valid = 1;
        printf("Enter round %d end time in HH:MM \n", i + 1);
        scanf("%s", time);
    
        for (int j = 0; j < strlen(time); j++) {
            if (!((time[j] >= '0' && time[j] <= '9') || (time[j] == ':'))) {
                printf("Invalid format \n");
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
            printf("The time you entered is invalid, please enter in a valid HH:MM format\n");
            i--;
            continue;
        }


        endtime[i] = h + (m / 60.0);
    }
    }



}