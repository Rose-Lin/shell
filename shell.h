#include "parser.h"

void free_parser(parse_output*);

void init_joblists(); // checked

void free_joblists(); // checked

void print_jobs(dlist job);  // checked

char* read_input();

// signals
int set_up_signals(); // sets up the signals for the shell

void sigchld_handler(int, siginfo_t*, void*); // signal handler for sigchld

int update_list(pid_t gid, int flag);

void test_job_list(); //testing joblist

//void* create_shared_memory(size_t size);

int perform_task(parser_output*);

int execute_bg(char*);

int execute_fg(char*);

int bring_tofg(parser_output* p)

//int execute_input(char* task);
