#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include "pcb.h"
#include "kernel.h"
#include "shell.h"
#include "shellmemory.h"
#include "interpreter.h"
#include "ready_queue.h"
#include "interpreter.h"

bool active = false;
bool debug = false;
bool in_background = false;

#define MAX_LINE_LENGTH 1024

/**
 * DESCRIPTION:
 * Takes a file a parses by lines (ignoring empty lines)
 *
 */
static char **parseFile(FILE *file)
{
    int capacity = 10;
    char **lines = malloc(capacity * sizeof(char *));
    int count = 0;

    char line[MAX_LINE_LENGTH];
    while (fgets(line, MAX_LINE_LENGTH, file) != NULL)
    {
        size_t len = strlen(line);
        if (len > 0 && (line[len - 1] == '\n' || line[len - 1] == '\r'))
            line[len - 1] = '\0'; // Remove newline character if present
        if (line[0] == '\0')
            continue;
        lines[count] = strdup(line);
        count++;
        if (count >= capacity)
        {
            capacity *= 2;
            lines = realloc(lines, capacity * sizeof(char *));
        }
    }

    // Allocate one more space for NULL terminator
    lines = realloc(lines, (count + 1) * sizeof(char *));
    lines[count] = NULL; // Null-terminate the array
    return lines;
}

/**
 * DESCRIPTION:
 * Helper for getting length of a char array
 *
 * arr must be NULL terminated
 *
 */
static int get_arr_length(char **arr)
{
    int len = 0;
    for (; arr[len] != NULL; len++)
        ;
    return len;
}

/**
 * DESCRIPTION:
 * Frees char* array
 *
 * arr MUST be NULL terminated
 */
static void free_str_arr(char **arr)
{
    for (int i = 0; arr[i] != NULL; i++)
    {
        free(arr[i]);
    }
    free(arr);
}

/**
 * DESCRIPTION:
 * Copies file into backing directory, and then opens new file
 */
FILE *copy_file_to_backing_dir(const char *filename)
{
    char cp_cmd[2000];
    cp_cmd[0] = '\0';

    // copies file
    strcat(cp_cmd, "cp ");
    strcat(cp_cmd, filename);
    strcat(cp_cmd, " ./backing");
    system(cp_cmd);

    // opens newly created file
    cp_cmd[0] = '\0';
    strcat(cp_cmd, "./backing/");
    strcat(cp_cmd, filename);

    FILE *fp = fopen(filename, "rt");
    return fp;
}

#define DEFAULT_PAGE_LOAD 2

// static int load_next_lines(char **lines, int page_load_count,)

// Must modify this function to copy the file into the backing store, close it, then open
// the new file
int process_initialize(const char *filename)
{

    // checks if files exists
    FILE *fp = fopen(filename, "rt");
    if (fp == NULL)
        return FILE_DOES_NOT_EXIST;
    fclose(fp);

    // copies file to backing store
    fp = copy_file_to_backing_dir(filename);


    char **lines = parseFile(fp);
    int line_count = get_arr_length(lines);
    int *page_tbl = malloc(sizeof(int) * line_count);
    for(int i=0; i < line_count; i++)
        page_tbl[i] = -1;
    
    // number of lines to be loaded into memory
    int loaded_lines = line_count >= 3 ? 6 : 3;

    // creates new process
    PCB *newPCB = makePCB(0, line_count - 1, filename);
    newPCB->lines = lines;
    newPCB->line_count = line_count;
    
    // loads line into memory
    int error_code = load_lines(lines, line_count, 0, loaded_lines, newPCB->pid, page_tbl);

    if (error_code != 0)
    {
        fclose(fp);
        free_str_arr(lines);
        freePCB(newPCB);
        return FILE_ERROR;
    }

    QueueNode *node = malloc(sizeof(QueueNode));
    node->pcb = newPCB;
    node->page_tbl = page_tbl;

    ready_queue_add_to_tail(node);

    fclose(fp);
    return 0;
}

int shell_process_initialize()
{
    // Note that "You can assume that the # option will only be used in batch mode."
    // So we know that the input is a file, we can directly load the file into ram
    int start;
    int end;
    int error_code = 0;

    int *page_tbl = malloc(sizeof(int) * VAR_MEM_SIZE / PAGES_PER_FRAME);

    PCB *newPCB = makePCB(start, 0, "_SHELL");
    newPCB->priority = true;

    error_code = load_lines(NULL, 0, 0, 0, newPCB->pid, page_tbl);
    if (error_code != 0)
    {
        freePCB(newPCB);
        return error_code;
    }

    QueueNode *node = malloc(sizeof(QueueNode));
    node->pcb = newPCB;
    node->page_tbl = page_tbl;

    ready_queue_add_to_head(node);

    freopen("/dev/tty", "r", stdin);
    return 0;
}

bool execute_process(QueueNode *node, int quanta)
{
    char *line = NULL;
    PCB *pcb = node->pcb;
    for (int i = 0; i < quanta; i++)
    {
        
        line = mem_get_value_at_line(pcb->PC, node->page_tbl, pcb->pid);

        // Requested line is not in memory, therefore we need to load it into memory
        if(!line) {
            int nb_pages = 3;

            load_lines(
                pcb->lines,
                pcb->line_count,
                pcb->PC,
                nb_pages,
                pcb->pid,
                node->page_tbl
            );   

            return false;
        }

        pcb->PC++;
        in_background = true;
        if (pcb->priority)
        {
            pcb->priority = false;
        }
        if (pcb->PC > pcb->end)
        {
            parseInput(line);
            terminate_process(node);
            in_background = false;
            return true;
        }


        parseInput(line);
        in_background = false;

    }
    return false;
}

void *scheduler_FCFS()
{
    QueueNode *cur;
    while (true)
    {
        if (is_ready_empty())
        {
            if (active)
                continue;
            else
                break;
        }
        cur = ready_queue_pop_head();
        execute_process(cur, MAX_INT);
    }
    return 0;
}

void *scheduler_SJF()
{
    QueueNode *cur;
    while (true)
    {
        if (is_ready_empty())
        {
            if (active)
                continue;
            else
                break;
        }
        cur = ready_queue_pop_shortest_job();
        execute_process(cur, MAX_INT);
    }
    return 0;
}

void *scheduler_AGING_alternative()
{
    QueueNode *cur;
    while (true)
    {
        if (is_ready_empty())
        {
            if (active)
                continue;
            else
                break;
        }
        cur = ready_queue_pop_shortest_job();
        ready_queue_decrement_job_length_score();
        if (!execute_process(cur, 1))
        {
            ready_queue_add_to_head(cur);
        }
    }
    return 0;
}

void *scheduler_AGING()
{
    QueueNode *cur;
    int shortest;
    sort_ready_queue();
    while (true)
    {
        if (is_ready_empty())
        {
            if (active)
                continue;
            else
                break;
        }
        cur = ready_queue_pop_head();
        shortest = ready_queue_get_shortest_job_score();
        if (shortest < cur->pcb->job_length_score)
        {
            ready_queue_promote(shortest);
            ready_queue_add_to_tail(cur);
            cur = ready_queue_pop_head();
        }
        ready_queue_decrement_job_length_score();
        if (!execute_process(cur, 1))
        {
            ready_queue_add_to_head(cur);
        }
    }
    return 0;
}

void *scheduler_RR(void *arg)
{
    int quanta = ((int *)arg)[0];
    QueueNode *cur;
    while (true)
    {
        if (is_ready_empty())
        {
            if (active)
                continue;
            else
                break;
        }
        cur = ready_queue_pop_head();
        if (!execute_process(cur, quanta))
        {
            ready_queue_add_to_tail(cur);
        }
    }
    return 0;
}

int schedule_by_policy(char *policy)
{ //, bool mt){
    if (strcmp(policy, "FCFS") != 0 && strcmp(policy, "SJF") != 0 &&
        strcmp(policy, "RR") != 0 && strcmp(policy, "AGING") != 0 && strcmp(policy, "RR30") != 0)
    {
        return SCHEDULING_ERROR;
    }
    if (active)
        return 0;
    if (in_background)
        return 0;
    int arg[1];
    if (strcmp("FCFS", policy) == 0)
    {
        scheduler_FCFS();
    }
    else if (strcmp("SJF", policy) == 0)
    {
        scheduler_SJF();
    }
    else if (strcmp("RR", policy) == 0)
    {
        arg[0] = 2;
        scheduler_RR((void *)arg);
    }
    else if (strcmp("AGING", policy) == 0)
    {
        scheduler_AGING();
    }
    else if (strcmp("RR30", policy) == 0)
    {
        arg[0] = 30;
        scheduler_RR((void *)arg);
    }

    return 0;
}
