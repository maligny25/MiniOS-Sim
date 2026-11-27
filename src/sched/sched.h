#ifndef SCHED_H
#define SCHED_H

#define MAX_PROCS 128
#define MAX_BURSTS 64

typedef enum {
    STATE_NEW,
    STATE_READY,
    STATE_RUNNING,
    STATE_WAITING,
    STATE_TERMINATED
} ProcessState;

typedef struct {
    char pid[32];
    int arrival;
    int orig_priority;
    int curr_priority;

    int bursts[MAX_BURSTS];
    int num_bursts;
    int curr_burst_idx;
    int remaining_time;

    ProcessState state;
    int ready_enter_time;
    int first_run_time;
    int finish_time;
    int waiting_time;
} Process;

int load_processes(const char *filepath, Process procs[], int *count);

int run_scheduler(Process procs[], int count, const char *policy, int trace, int csv);

int run_compare(const Process orig_procs[], int count);

#endif