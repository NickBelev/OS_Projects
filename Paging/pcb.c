#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "pcb.h"

int pid_counter = 1;

int generatePID(){
    return pid_counter++;
}

//In this implementation, Pid is the same as file ID 
PCB* makePCB(int start, int end, const char *processname){
    PCB * newPCB = malloc(sizeof(PCB));
    newPCB->pid = generatePID();
    newPCB->PC = start;
    newPCB->start  = start;
    newPCB->end = end;
    newPCB->job_length_score = 1+end-start;
    newPCB->priority = false;

    newPCB->lines = NULL;
    newPCB->line_count=0;

    newPCB->process_name=strdup(processname);
    return newPCB;
}

// frees PCB 
void freePCB(PCB *pcb) {
    for(int i=0; i < pcb->line_count; i++) {
        free(pcb->lines[i]);
    }
    free(pcb->lines);
    free(pcb->process_name);
    free(pcb);
}

