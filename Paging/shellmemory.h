#ifndef SHELLMEMORY_H
#define SHELLMEMORY_H

// When pushing to main MAKE SURE THAT THESE MACROS ARE COMMENTED
// When working on the assignment these can be used

// #define PAGE_FRAME_COUNT 6
// #define VAR_MEM_SIZE 10 // must be multiple of 3
#define PAGES_PER_FRAME 3

void print_framestore();

void free_process_page(int PG, int *page_tbl);

void init_framestore();
void free_framestore();

void varmemory_init();
void varmemory_free();

char *mem_get_value(const char *var);
void mem_set_value(const char *var, const char *value);
int load_lines(char **lines, int total_lines_nb, int offset, int line_count, int pid, int *page_tbl);

char *mem_get_value_at_line(int PC, int *page_tbl, int pid);

void mem_free_lines_between(int start, int end, int *pagetbl);

void print_mem_stats();

#endif