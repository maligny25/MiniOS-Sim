#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sched.h"

typedef struct{
    char pid[32];
    int start;
    int end;
} GanttBlock;

int load_processes(const char *filepath, Process procs[], int *count) {
    FILE *fp= fopen(filepath,"r");

    if (fp == NULL) {
        fprintf(stderr,"File can not be opened.\n");
        return 1;
    }

    char line[512];
    *count = 0;

    while(fgets(line,sizeof(line),fp)) {
        if (line[0]=='#' || line[0] == '\n' || line[0] == '\r'){
            continue;
        }

        if (*count >= MAX_PROCS) {
            fprintf(stderr, "Error: Too many processes\n");
            fclose(fp);
            return 1;
        }

        Process *p = &procs[*count];

        char *tok= strtok(line, " \t\r\n");
        if (tok==NULL || tok[0] == '#') continue;
        strncpy(p->pid, tok, 31);
        p->pid[31] = '\0';

        tok = strtok(NULL, " \t\r\n");
        if(tok == NULL) {
            fprintf(stderr,"Error: Missing Line\n");
            fclose(fp);
            return 1;
        }
        p->arrival = atoi(tok);
        

        tok = strtok(NULL, " \t\r\n");
        if(tok == NULL) {
            fprintf(stderr,"Error: Missing Line\n");
            fclose(fp);
            return 1;
        }
        p->orig_priority = atoi(tok);
        p->curr_priority = atoi(tok);
        
        p->num_bursts = 0;
        while((tok = strtok(NULL, " \t\r\n")) != NULL) {
            if (p->num_bursts >= MAX_BURSTS) {
                fprintf(stderr, "Error : Too many bursts\n");
                fclose(fp);
                return 1;
            }
            p->bursts[p->num_bursts++] = atoi(tok);
        }
        if(p->num_bursts == 0 || p->num_bursts % 2 == 0){
            fprintf(stderr,"Error: Invalid burst sequence\n");
            fclose(fp);
            return 1;
        }
        p->curr_burst_idx = 0;
        p->remaining_time = p->bursts[0];
        p->state = STATE_NEW;
        p->ready_enter_time = -1;
        p->first_run_time = -1;
        p->finish_time = 0;
        p->waiting_time = 0;
        (*count)++;
    }
    fclose(fp);
    return 0;
}

static void sort_by_pid(Process procs[], int count) {
    for (int i = 0 ; i < count-1; i++) {
        for (int j = 0 ; j < count-i-1; j++){
            if (strcmp(procs[j].pid, procs[j+1].pid) > 0){
                Process tmp = procs[j];
                procs[j] = procs[j+1];
                procs[j+1] = tmp;
            }
        }
    }
}

static int simulate_core(Process procs[], int count, const char *policy, int trace, int csv, int quiet,
                         double *out_turn, double *out_wait, double *out_resp) {
    if(strcmp(policy, "fcfs") != 0 &&
       strcmp(policy, "sjf") != 0 &&
       strcmp(policy, "srtf") != 0 &&
       strcmp(policy, "priority") != 0 &&
       strcmp(policy, "rr") != 0) {
       fprintf(stderr, "Error: Unknown scheduling policy '%s'\n", policy);
       return 1;
    }

    int rr_slice = 0;

    sort_by_pid(procs, count);

    GanttBlock gantt[2048];
    int gantt_count = 0;
    int tick = 0;
    int terminated_count = 0;
    int running_idx = -1;

    while(terminated_count < count) {
        for (int i = 0; i < count; i++) {
            if (procs[i].state == STATE_NEW && procs[i].arrival == tick) {
                procs[i].state = STATE_READY;
                procs[i].ready_enter_time = tick;
                if (trace) printf("[tick %d] %s arrived -> READY\n", tick, procs[i].pid);
            } else if (procs[i].state == STATE_WAITING && procs[i].remaining_time == 0) {
                procs[i].curr_burst_idx++;
                procs[i].remaining_time = procs[i].bursts[procs[i].curr_burst_idx];
                procs[i].state = STATE_READY;
                procs[i].ready_enter_time = tick;
                if (trace) printf("[tick %d] %s I/O done -> READY\n", tick, procs[i].pid);
            }
        }

        if (running_idx != -1 && procs[running_idx].remaining_time > 0) {
            if (strcmp(policy, "srtf") == 0) {
                for (int i = 0; i < count; i++) {
                    if (procs[i].state == STATE_READY &&
                        procs[i].remaining_time < procs[running_idx].remaining_time) {
                        if (trace) printf("[tick %d] %s preempted by %s -> READY\n", tick, procs[running_idx].pid, procs[i].pid);
                        procs[running_idx].state = STATE_READY;
                        procs[running_idx].ready_enter_time = tick;
                        running_idx = -1;
                        break;
                    }
                }
            } else if(strcmp(policy, "priority") == 0) {
                for (int i = 0; i < count; i++) {
                    if (procs[i].state == STATE_READY &&
                        procs[i].curr_priority < procs[running_idx].curr_priority) {
                        if (trace) printf("[tick %d] %s preempted by %s -> READY\n", tick, procs[running_idx].pid, procs[i].pid);
                        procs[running_idx].state = STATE_READY;
                        procs[running_idx].ready_enter_time = tick;
                        running_idx = -1;
                        break;
                    }
                }
            } else if (strcmp(policy, "rr") == 0 && rr_slice == 4) {
                if (trace) printf("[tick %d] %s quantum expired -> READY\n", tick, procs[running_idx].pid);
                procs[running_idx].state = STATE_READY;
                procs[running_idx].ready_enter_time = tick;
                running_idx = -1;
                rr_slice = 0;
            }
        }

        if (running_idx != -1 && procs[running_idx].remaining_time == 0) {
            procs[running_idx].curr_burst_idx++;
            if (procs[running_idx].curr_burst_idx == procs[running_idx].num_bursts) {
                procs[running_idx].state = STATE_TERMINATED;
                procs[running_idx].finish_time = tick;
                terminated_count++;
                if (trace) printf("[tick %d] %s terminated\n", tick, procs[running_idx].pid);
            } else {
                procs[running_idx].state = STATE_WAITING;
                procs[running_idx].remaining_time = procs[running_idx].bursts[procs[running_idx].curr_burst_idx];
                if (trace) printf("[tick %d] %s CPU burst done -> WAITING (I/O=%d)\n", tick, procs[running_idx].pid, procs[running_idx].remaining_time);
            }
            running_idx = -1;
            rr_slice = 0;
        }

        if (running_idx == -1 && terminated_count < count) {
            int best_idx = -1;
            for (int i = 0; i < count; i++) {
                if (procs[i].state != STATE_READY) continue;

                if (best_idx == -1) {
                    best_idx = i;
                    continue;
                }

                int take_new = 0;
                if (strcmp(policy, "fcfs") == 0 || strcmp(policy, "rr") == 0) {
                    if (procs[i].ready_enter_time < procs[best_idx].ready_enter_time)
                        take_new = 1;
                } else if (strcmp(policy, "sjf") == 0 || strcmp(policy, "srtf") == 0) {
                    if (procs[i].remaining_time < procs[best_idx].remaining_time ||
                        (procs[i].remaining_time == procs[best_idx].remaining_time &&
                         procs[i].ready_enter_time < procs[best_idx].ready_enter_time))
                         take_new = 1;
                } else if (strcmp(policy, "priority") == 0) {
                    if (procs[i].curr_priority < procs[best_idx].curr_priority ||
                        (procs[i].curr_priority == procs[best_idx].curr_priority &&
                        procs[i].ready_enter_time < procs[best_idx].ready_enter_time))
                        take_new = 1;
                }

                if (take_new) best_idx = i;
            }

            if (best_idx != -1) {
                running_idx = best_idx;
                procs[running_idx].state = STATE_RUNNING;
                rr_slice = 0;
                if (procs[running_idx].first_run_time == -1) {
                    procs[running_idx].first_run_time = tick;
                }
                if (trace) printf("[tick %d] dispatch %s (rem=%d, prio=%d)\n", tick, procs[running_idx].pid, procs[running_idx].remaining_time, procs[running_idx].curr_priority);
            }
        }

        for (int i = 0; i < count; i++) {
            if (procs[i].state == STATE_READY) {
                procs[i].waiting_time++;
                if (strcmp(policy, "priority") == 0 &&
                    procs[i].waiting_time % 5 == 0 &&
                    procs[i].curr_priority > 0) {
                    procs[i].curr_priority--;
                    if (trace) printf("[tick %d] %s aged -> prio=%d\n", tick + 1, procs[i].pid, procs[i].curr_priority);
                }
            } else if (procs[i].state == STATE_RUNNING){
                procs[i].remaining_time--;
                rr_slice++;
            } else if (procs[i].state == STATE_WAITING) {
                procs[i].remaining_time--;
            }
        }

        if (terminated_count == count) {
            break;
        }

        const char *curr_pid = (running_idx != -1) ? procs[running_idx].pid : "IDLE";

        if (gantt_count > 0 &&
            strcmp(gantt[gantt_count-1].pid, curr_pid) == 0 &&
            gantt[gantt_count - 1].end == tick) {
            gantt[gantt_count - 1].end = tick + 1;
        } else {
            strcpy(gantt[gantt_count].pid, curr_pid);
            gantt[gantt_count].start = tick;
            gantt[gantt_count].end = tick + 1;
            gantt_count++;
        }

        tick++;
    }

    double sum_turn = 0, sum_wait = 0, sum_resp = 0;
    for (int i = 0; i < count; i++) {
        int turnaround = procs[i].finish_time - procs[i].arrival;
        int response = procs[i].first_run_time - procs[i].arrival;
        sum_turn += turnaround;
        sum_wait += procs[i].waiting_time;
        sum_resp += response;
    }

    if (out_turn) *out_turn = sum_turn / count;
    if (out_wait) *out_wait = sum_wait  / count;
    if (out_resp) *out_resp = sum_resp / count;

    if (quiet) return 0;
    
    if (csv) {
        printf("pid,arrival,finish,turnaround,waiting,response\n");
        for (int i = 0; i < count; i++){
            int turnaround = procs[i].finish_time - procs[i].arrival;
            int response = procs[i].first_run_time - procs[i].arrival;
            printf("%s,%d,%d,%d,%d,%d\n",
                    procs[i].pid,procs[i].arrival,procs[i].finish_time,
                    turnaround, procs[i].waiting_time,response);
        }
        return 0;
    }

    printf("Gantt: ");
    for (int i = 0; i < gantt_count; i++) {
        printf("%s %d-%d%s", gantt[i].pid, gantt[i].start, gantt[i].end,
        (i < gantt_count - 1) ? " | " : "\n");
    }
    

    printf("Avg Turnaround: %.2f | Avg Waiting: %.2f | Avg Response: %.2f\n",
            sum_turn / count, sum_wait / count, sum_resp / count);

    return 0;
}

int run_scheduler(Process procs[], int count, const char *policy, int trace, int csv){
    return simulate_core(procs, count, policy, trace, csv, 0, NULL, NULL, NULL);
}

int run_compare(const Process orig_procs[], int count) {
    const char *policies[5]= {"fcfs", "sjf", "srtf", "priority", "rr"};
    Process copy[MAX_PROCS];

    printf("%-10s %15s %15s %15s\n", "Policy", "Avg Turnaround", "Avg Waiting", "Avg Response");
    printf("-----------------------------------------------------------\n");

    for (int p = 0; p < 5; p++) {
        memcpy(copy, orig_procs, sizeof(Process) * count);
        double avg_turn = 0, avg_wait = 0, avg_resp = 0;
        if (simulate_core(copy, count, policies[p], 0, 0, 1, &avg_turn, &avg_wait, &avg_resp) != 0) {
            return 1;
        }
        printf("%-10s %15.2f %15.2f %15.2f\n", policies[p], avg_turn, avg_wait, avg_resp);
    }
    return 0;
}