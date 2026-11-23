#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "sched.h"

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



