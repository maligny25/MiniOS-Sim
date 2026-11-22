#include <stdio.h>
#include <string.h>

int main(int argc, char *argv[]) {
    if (argc < 5) {
        fprintf(stderr, "Error: Not enough arguments provided.\n");
        return 1;
    }

    if(strcmp(argv[1], "sched") == 0 && strcmp(argv[2], "a") == 0 && strcmp(argv[3], "fcfs") == 0) {
        printf("Command applied! File: %s\n", argv[4]);
        return 0;
    }

    fprintf(stderr, "Error: Invalid command or arguments.\n");
    return 1;
}