#include "sched/sched.h"
#include <stdio.h>
#include <string.h>

int main(int argc, char *argv[]) {
    if (argc < 5) {
        fprintf(stderr, "Error: Not enough arguments provided.\n");
        return 1;
    }

    if(strcmp(argv[1], "sched") == 0 && strcmp(argv[2], "a") == 0) {
        int trace = 0;
        int csv = 0;
        int compare = 0;
        for (int i = 5; i < argc; i++) {
            if (strcmp(argv[i], "--trace") == 0) {
                trace = 1;
            } else if (strcmp(argv[i],"--csv") == 0){
                csv = 1;
            } else if (strcmp(argv[i],"--compare") == 0){
                compare = 1;
            } else {
                fprintf(stderr, "Error: Unknown flag %s\n", argv[i]);
                return 1; 
            }
        }

        Process procs[MAX_PROCS];
        int count = 0;
        if (load_processes(argv[4], procs, &count) != 0){
            return 1;
        }
        
        if (compare) {
            return run_compare(procs, count);
        }
        return run_scheduler(procs, count, argv[3], trace, csv);
    }

    fprintf(stderr, "Error: Invalid command or arguments.\n");
    return 1;
}