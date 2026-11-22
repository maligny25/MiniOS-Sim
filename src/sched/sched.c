#include <stdio.h>
#include "sched.h"

int run_fcfs_first(const char *filepath) {
    FILE *fp = fopen(filepath, "r");
    if (fp == NULL) {
        fprintf(stderr, "Error: Could not open file %s\n", filepath);
        return 1;
    }
    char line[256];
    char pid[32]; int arrival, priority, burst;
    while (fgets(line, sizeof(line), fp)) {
        if(line[0] == '#' || line[0] == '\n' || line[0] == '\r') {
            continue;
        }
        if (sscanf(line, "%31s %d %d %d", pid, &arrival, &priority, &burst) == 4) {
            break;
        }
    }
    (void)priority;
    fclose(fp);
    printf("%s %d-%d\n",pid, arrival, arrival + burst);
    return 0;
}

