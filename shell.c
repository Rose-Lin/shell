#include <sys/types.h>
#include <termios.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/mman.h>
#include <semaphore.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <signal.h>
#include <termios.h>

#include "job_node.h"
#include "dlist.h"
#include "shell.h"
#include "tokenizer.h"

#define FALSE 0
#define TRUE 1
#define BUFSIZE 20
#define NOTKNOWN -1

// enums
enum status{background, foreground, suspended};
enum flags{fg_to_bg,// not needed
	   sus_to_bg,
	   bg_to_fg,
	   exit_shell,
	   start_bg};
enum special_inputs{delim_bg = '&', delim_mult = ';'};
// semaphores
sem_t* all;
sem_t* job; //semaphore for suspended or background jobs

// job lists
dlist all_joblist;
dlist sus_bg_jobs;

// globals
char special_delim[] = "&;";
char all_delim[] = " &;";

int multi_jobs = FALSE;
int launch_bg = FALSE;

// needs to be guarded
pid_t to_update; // keeps track of the pid of the current job to be updated

int job_num;  // used to track the job index
int execjob_num = 0; // number of jobs to be executed during one input

// terminal attribute related globals
pid_t shell_pid;
struct termios mysh;
int mysh_fd = STDIN_FILENO; 


void int_sems() {
	all = sem_open("all", O_CREAT, 0666, 1);
	bglist = sem_open("bglist", O_CREAT, 0666, 1);
	suslist = sem_open("suslist", O_CREAT, 0666, 1);
}

void close_sems() {
	sem_unlink("all");
	sem_close(all);
	sem_unlink("bglist");
	sem_close(bglist);
	sem_unlink("suslist");
	sem_close(suslist);
}

void init_joblists() {
	job_num = 0;
	all_joblist = dlist_new();
	sus_bg_jobs = dlist_new();
}

void free_joblists() {
	dlist_free(sus_bg_jobs);
	dlist_free(all_joblist);
}

void print_jobs(dlist jobs) {
	job_node* top = get_head(jobs);
	if(jobs == NULL) {
		printf("No jobs available.\n");
	} else if (top == NULL) {
		printf("No jobs yet. \n");
	}
	int index = 1;
	while(top != NULL) {
		printf("%d. %s \n", index, get_input(top));
		top = top->next;
		index ++;
	}
}

/*
void sigchild_handler() {
	cur_fg_job = STOUT_FILENO;
}

void update_list(pid_t gid, int flag) {

}
*/


// read in the input and add one jobnode(with original input)
char* read_input() {
  size_t readn;
  char* input = NULL;
  printf("mysh > ");
  if(getline(&input, &readn, stdin) < 0) {
    printf("getline from stdin failed! \n");
    free(input);
    input = NULL;
  }
	job_num ++;
	job_node* jn = new_node(job_num, NOTKNOWN, NOTKNOWN, NOTKNOWN, NULL, NULL, get_tail(all_joblist));
	jn->original_input = malloc(sizeof(char) * (strlen(input) + 1));
	strncpy(jn->original_input, input, strlen(input));
	jn->original_input[strlen(input)] = '\n';
	dlist_push_end(all_joblist, jn);
  return input;
}

int check_special_symbols(char* input) {
	execjob_num = 0;
	struct tokenizer* t = init_tokenizer(input, special_delim);
	char* token = get_next_token(t);
	while(token != NULL) {
		execjob_num += 1;
		token = get_next_token(t);
	}
	free_tokenizer(t);
	return execjob_num;
}

/*
void test_job_list(){
  dlist dl = dlist_new();
  job_node* j1 = new_node(1, 0,0,0,"first job",NULL, NULL);
  dlist_push_end(dl, j1);
  job_node* j2 = new_node(2,0,0,0,"second job", NULL, NULL);
  insert_after(j1, j2);
  job_node* j3 = new_node(3,0,0,0,"thrid job", NULL, NULL);
  insert_after(j2,j3);
  job_node* j4 = new_node(4,0,0,0, "fourth job", NULL,NULL);
  insert_after(j3, j4);
  job_node* j5 = new_node(5,0,0,0, "5th job", NULL,NULL);
  insert_after(j4, j5);
  job_node* j12 = new_node(0,0,0,0,"after 1st before 2nd", NULL,NULL);
  job_node* h = j1;
  dlist_push_end(dl, j2);
  printf("%s\n",(dl->tail)->original_input );
  // while (h){
    // dlist_push_end(dl, h);
    // printf(h->original_input);
    // printf("\n" );
    // h = h->next;
  // }
  printf("%d\n",dlist_size(dl) );
  dlist_insert(dl,1, j12);
  dlist_remove(dl, 2);
  h = j1;
  // while (h){
  //   printf(h->original_input);
  //   printf("\n" );
  //   h = h->next;
  // }
  delete_node(j1);
}
*/

int parse_input(char* input, char* delim, char** tasks) {
	char* cur = input;
	int total = 0;
	int size = BUFSIZE;
	struct tokenizer* t = init_tokenizer(input, delim);
	char* token = get_next_token(t);
	tasks = malloc(sizeof(char*) * size);

	while(token != NULL) {
		int strlength = strlen(cur) - strlen(token);
		int malloclength = strlength + 1;
		if(*token == '&') { //if the current one is background job
			malloclength ++;
		}
		char* cur_token = malloc(sizeof(char) *  malloclength);
		strncpy(cur_token, cur, strlength);
		if(*token == '&') {
				cur_token += '&';
		}
		cur_token[malloclength - 1] = '\n';
		tasks[total] = cur_token;

		total += 1;
		if(total >= BUFSIZE) {
			size += BUFSIZE;
			tasks = (char**)realloc(tasks, size);
		}
		token = get_next_token(t);
	}
	return total;
}

int set_up_signals() {
	sigset_t shellmask;
	struct sigaction sa;

	sigaddset(&shellmask, SIGINT);
	sigaddset(&shellmask, SIGTERM);
	sigaddset(&shellmask, SIGTTOU);
	sigaddset(&shellmask, SIGTTIN);
	sigaddset(&shellmask, SIGQUIT);
	sigaddset(&shellmask, SIGTSTP);
	sigprocmask(SIG_BLOCK,& shellmask, NULL);

	sa.sa_flags = SA_SIGINFO;
	sa.sa_sigaction = &sigchld_handler;
	sigaction(SIGCHLD, &sa, NULL);

}



int execute_input(char* task) {
	char** processes;
	int argnum = parse_input(task, " ", processes);
	if(strcmp(processes[0], "jobs") == 0) {
		print_jobs(sus_bg_jobs);
		return TRUE;
	} else if(strcmp(processes[0], "bg") == 0) {
		printf("to be implemented\n");	
	} else if(strcmp(processes[0], "fg") == 0) {
		printf("to be implemented\n");
	} else if (strcmp(processes[0], "kill") == 0) {
		printf("to be implemented\n");
	} else {
		printf("not yet\n");
	}
	return TRUE;
}


int main(int argc, char* argv[]){
	// sets up
	int run = FALSE;
	tcgetattr(mysh_fd, &mysh);
	shell_pid = getpid();
	set_up_signals();

	// starts executing
	char* input = read_input();
	char** multi_jobs; // needs to free
	int num_jobs = parse_input(input, ";", multi_jobs);

	do {
		for (int i = 0; i < num_jobs; i++) {
			char** curjob;  //needs to free
			int jobnum = parse_input(multi_jobs[i], "&", curjob);
			for(int j = 0; j < jobnum; j++) {
				run = execute_input(curjob[0]);
			}
		}
	} while (run);
}





void* create_shared_memory(size_t size){
  int protection = PROT_READ | PROT_WRITE;
  int visibility = MAP_ANONYMOUS | MAP_SHARED;
  return mmap(NULL, size, protection, visibility, 0, 0);
}

/*
int i = 0;
void* shmem_all_joblist = create_shared_memory(sizeof(dlist*));
void* shmem_background_joblist = create_shared_memory(sizeof(dlist*));
void* shmem_suspended_joblist = create_shared_memory(sizeof(dlist*));
memcpy(shmem_all_joblist, (void*)&all_joblist, sizeof(dlist*));
memcpy(shmem_suspended_joblist,(void*)&suspended_joblist,sizeof(dlist*));
memcpy(shmem_background_joblist,(void*)&background_joblist,sizeof(dlist*));
size_t byte_num = 0;
size_t buff_size = BUFSIZE;
char* buffer = NULL;
// byte_num = getline(&buffer, &buff_size, STDIN_FILENO);
char* delimiter = ";& ";
tokenizer* t = init_tokenizer("se xdfnsfeos;skei&siefn;se", delimiter);
printf("%s\n", get_next_token(t));
test_job_list();
*/
