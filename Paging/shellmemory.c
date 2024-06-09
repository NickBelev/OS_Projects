#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <limits.h>
#include <stdbool.h>
#include "shellmemory.h"

/**
 * Implementation for frame store
 */
typedef struct Page
{
    int *pid; // keeps track of process
    char *line; // line that is stored
    int PC; // allows us to uniquely identify lines with their process (PID)

    int accessTime; // used in our LRU policy
} Page;

// stores all the frames
Page framestore[PAGE_FRAME_COUNT];

// Tell use which page is is use
bool page_is_used[PAGE_FRAME_COUNT / PAGES_PER_FRAME];

// Variables for keeping track of Number of pages and number of frames
const int TotalPageCount = PAGE_FRAME_COUNT;
const int TotalFrames = PAGE_FRAME_COUNT / PAGES_PER_FRAME;

static int globalClock = 0; // global clock for LRU


/**
 * DESCRIPTION:
 * initializes frame store
 */
void init_framestore()
{
    for (int i = 0; i < TotalPageCount; i++)
    {
        framestore[i].line = NULL;
        framestore[i].pid = NULL;
        framestore[i].PC = -1;
        framestore[i].accessTime = 0;
    }

    for (int i = 0; i < TotalFrames; i++)
    {
        page_is_used[i] = false;
    }
}

/**
 * DESCRIPTION:
 * Frees framestore, resetting to its default values
 * And resets page_is_used array to false
 */
void free_framestore()
{   
    // resets pages
    for (int i = 0; i < TotalPageCount; i++)
    {
        if (framestore[i].line != NULL)
        {
            free(framestore[i].line);
        }

        if (framestore[i].pid != NULL)
        {
            free(framestore[i].pid);
        }

        framestore[i].line = NULL;
        framestore[i].pid = NULL;
        framestore[i].PC = -1;
        framestore[i].accessTime = 0;
    }

    // resets page is used array to false
    for(int i =0; i < TotalFrames; i++) {
        page_is_used[i]=false;
    }
}


/**
 * Handles logic for finding page to evict, and setting the candidates array
 * 
*/
int evict_page(int *candidate_pages, int length)
{
    int oldestTime = INT_MAX;
    int oldestIndex = -1;

    // Find LRU page
    for (int i = 0; i < TotalPageCount; i++)
    {
        if (framestore[i].accessTime < oldestTime && framestore[i].line != NULL)
        { // Check page is used
            oldestTime = framestore[i].accessTime;
            oldestIndex = i - (i % PAGES_PER_FRAME); // Ensure frame start
        }
    }

    // Evict LRU frame
    printf("Page fault! Victim page contents:\n");

    for (int i = 0; i < PAGES_PER_FRAME; i++)
    {
        // Print & free the oldest frame's pages
        if (!framestore[oldestIndex + i].line)
            break;

        printf("%s\n", framestore[oldestIndex + i].line);
        free(framestore[oldestIndex + i].line);
        framestore[oldestIndex + i].line = NULL;
        free(framestore[oldestIndex + i].pid);
        framestore[oldestIndex + i].pid = NULL;
        framestore[oldestIndex + i].accessTime = 0; // Reset access time
        framestore[oldestIndex + i].PC = -1;
    }

    printf("End of victim page contents.\n");

    // Update page usage tracking if needed
    for (int i = 0; i < length && i < PAGES_PER_FRAME; i++)
    {
        candidate_pages[i] = oldestIndex + i;
    }

    return oldestIndex; // Successfully evicted the oldest frame
}


/**
 * DESCRIPTION:
 * Loads a some number of lines into the frame store
 *
 * DESCRIPTION:
 * lines: lines to puit into mem
 * total_lines_nb: total number of lines in the lines array
 * offset: which offset to access lines arr (should be 1 or 2)
 * line_count: the number of lines to put into the memory
 * pid: pid  or process associated with lines
 * page_tbl: the page table that we should populate
 */
int load_lines(char **lines, int total_lines_nb, int offset, int line_count, int pid, int *page_tbl)
{
    int error_code = 0;
    // char **lines = parseFile(fp);
    // int line_count = get_arr_length(lines);
    bool found_spot = false;

    // this array will store possible pages that can be used for storing code
    int candidate_pages[line_count];

    int candidate_ptr = 0;

    for (int i = 0; i < TotalFrames;)
    {
        if (page_is_used[i])
        {
            i++;
            continue;
        }

        candidate_pages[candidate_ptr++] = 3 * i;
        if (candidate_ptr == line_count || offset + candidate_ptr == total_lines_nb)
        {
            found_spot = true;
            break;
        }

        candidate_pages[candidate_ptr++] = 3 * i + 1;
        if (candidate_ptr == line_count || offset + candidate_ptr == total_lines_nb)
        {
            found_spot = true;
            break;
        }

        candidate_pages[candidate_ptr++] = 3 * i + 2;
        if (candidate_ptr == line_count || offset + candidate_ptr == total_lines_nb)
        {
            found_spot = true;
            break;
        }

        i++;
    }

    // if a spot cannot be found
    if (!found_spot)
    {
        evict_page(candidate_pages, line_count);
    }

    for (int i = 0; i < line_count; i++)
    {
        if (lines[offset + i] == NULL)
            break;

        int pageIndex = candidate_pages[i];

        // creates new entry in page
        framestore[pageIndex].line = strdup(lines[offset + i]);
        framestore[pageIndex].pid = malloc(sizeof(int));
        *framestore[pageIndex].pid = pid;
        framestore[pageIndex].PC = offset + i;
        // increments global clock (used in our LRU)
        framestore[pageIndex].accessTime = globalClock++;

        page_is_used[pageIndex / PAGES_PER_FRAME] = true;

        int cand_page = pageIndex;
        // updates page table
        page_tbl[offset + i] = cand_page;
    }

    return error_code;
}

/**
 * DESCRIPTION:
 * Gets line/page from frame store using page_tbl
 *
 * If pid no longer matches, then the page was replaces, in which case we return NULL
 */
char *mem_get_value_at_line(int PC, int *page_tbl, int pid)
{
    int index = page_tbl[PC];
    // means PC not yet been loaded into mem
    if (index == -1)
    {
        return NULL;
    }

    // page as been swapped
    if (framestore[index].pid[0] != pid)
    {
        return NULL;
    }

    // some parts process are in our memory, but not the one we are looking for
    if(framestore[index].PC != PC) 
    {
        return NULL;
    }

    framestore[index].accessTime = globalClock++;

    return framestore[index].line;
}

/**
 * DESCRIPTION:
 * Frees pages from start to end
 */
void mem_free_lines_between(int start, int end, int *pagetbl)
{
    for (int i = start; i <= end && i < TotalPageCount; i++)
    {
        free_process_page(i, pagetbl);
    }
}

/**
 * DESCRIPTION:
 * Helper for freeing page associated with a process by using its program counter (or index)
 * And its page_tbl;
 */
void free_process_page(int PG, int *page_tbl)
{
    int index = page_tbl[PG];
    if (framestore[index].line != NULL)
        free(framestore[index].line);

    if (framestore[index].pid != NULL)
        free(framestore[index].pid);

    framestore[index].line = NULL;
    framestore[index].pid = NULL;
    framestore[index].accessTime = 0;
}

/**
 * DESCRIPTION:
 * Helper for printing page store
 */
void print_framestore()
{
    int used_pages = 0;
    for (int i = 0; i < VAR_MEM_SIZE; i++)
    {
        if (!page_is_used[i / PAGES_PER_FRAME])
            continue;
        printf("Page %d: Process '%d', Line '%s'\n",
               i, *framestore[i].pid, framestore[i].line);

        used_pages++;
    }
    printf("%d out of %d Pages Used\n", used_pages, VAR_MEM_SIZE);
}

/**
 * Implementation for Variable store
 */

struct Variable_struct
{
    char *var;
    char *value;
};

struct Variable_struct varmemory[VAR_MEM_SIZE];
const int VariableMemCount = VAR_MEM_SIZE;
size_t var_count = 0;

void static print_varmemory()
{
    printf("Contents of varmemory array:\n");
    for (int i = 0; i < VAR_MEM_SIZE; ++i)
    {
        printf("varmemory[%d].var = %s\n", i, varmemory[i].var);
        printf("varmemory[%d].value = %s\n", i, varmemory[i].value);
    }
}

/**
 * DESCRIPTION: 
 * Initializes variable store
*/
void varmemory_init()
{
    int i;
    for (i = 0; i < VAR_MEM_SIZE; i++)
    {
        varmemory[i].var = NULL;
        varmemory[i].value = NULL;
    }
}

/**
 * DESCRIPTION:
 * Frees shell/variable memory
 */
void varmemory_free()
{
    for (int i = 0; i < VAR_MEM_SIZE; i++)
    {
        if (varmemory[i].var)
            free(varmemory[i].var);
        if (varmemory[i].value)
            free(varmemory[i].value);

        varmemory[i].var = NULL;
        varmemory[i].value = NULL;
    }
}

// Set key value pair
void mem_set_value(const char *var_in, const char *value_in)
{
    int i;
    for (i = 0; i < VAR_MEM_SIZE; i++)
    {
        if (!varmemory[i].var)
            continue;

        if (strcmp(varmemory[i].var, var_in) == 0)
        {
            free(varmemory[i].value);
            varmemory[i].value = strdup(value_in);
            return;
        }
    }

    // Value does not exist, need to find a free spot.
    for (i = 0; i < VAR_MEM_SIZE; i++)
    {
        if (varmemory[i].var == NULL)
        {

            varmemory[i].var = strdup(var_in);
            varmemory[i].value = strdup(value_in);
            var_count++;
            return;
        }
    }
}

// get value based on input key
// allocates memory for new string
char *mem_get_value(const char *var_in)
{
    int i;
    for (i = 0; i < VAR_MEM_SIZE; i++)
    {
        if (varmemory[i].var == NULL)
            continue;
        if (strcmp(varmemory[i].var, var_in) == 0)
        {
            return strdup(varmemory[i].value);
        }
    }
    return NULL;
}

/**
 * DESCRIPTION:
 * Prints out stats about frames and variables
 */
void print_mem_stats()
{
    printf("Frame Store Size = %d; Variable Store Size = %d\n", PAGE_FRAME_COUNT, VAR_MEM_SIZE);
}
