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
#include <error.h>
#include <sys/wait.h>

#include "job_node.h"
#include "dlist.h"
#include "shell.h"
#include "tokenizer.h"

#define FALSE 0
#define TRUE 1
#define BUFSIZE 20
#define NOTKNOWN -1
#define SUCCESS 0
#define FAILURE 1

// enums
enum status{background, foreground, suspended};
enum flags{ fg_to_sus,
		sus_to_bg,
		bg_to_fg,
	  terminated,
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

int job_num;  // used to track the job index // not used

// not used (xia mian zhe ge)
int execjob_num = 0; // number of jobs to be executed during one input

// terminal attribute related globals
pid_t shell_pid;
pid_t shell_gpid;
struct termios mysh;
int mysh_fd = STDIN_FILENO;

/* ========================= Initializing ========================== */
void init_sems() {
	all = sem_open("all", O_CREAT, 0666, 1);
	job = sem_open("job", O_CREAT, 0666, 1);
}

void close_sems() {
	sem_unlink("all");
	sem_close(all);
	sem_unlink("job");
	sem_close(job);
}

void init_joblists() {
	job_num = 0;
	all_joblist = dlist_new();
	sus_bg_jobs = dlist_new();
}


/* ========================== Handle Signals ============================== */
void* sigchld_handler(int signal, siginfo_t* sg, void* oldact) {
	pid_t childpid = sg->si_pid;
	int status = sg->si_code;
	sigset_t sset;
	sigaddset(&sset, SIGCHLD);

	if(status == CLD_EXITED) {

		sigprocmask(SIG_BLOCK, &sset, NULL);
		update_list(childpid, terminated);
		sigprocmask(SIG_UNBLOCK, &sset, NULL);
		return NULL;

	} else if (status == CLD_KILLED) {

		sigprocmask(SIG_BLOCK, &sset, NULL);
		update_list(childpid, terminated);
		sigprocmask(SIG_UNBLOCK, &sset, NULL);
		return NULL;

	} else if (status == CLD_STOPPED) {

		sigprocmask(SIG_BLOCK, &sset, NULL);
		update_list(childpid, fg_to_sus);
		sigprocmask(SIG_UNBLOCK, &sset, NULL);
		return NULL;

	} else if (status == CLD_CONTINUED) {

		sigprocmask(SIG_BLOCK, &sset, NULL);
		update_list(childpid, bg_to_fg);
		sigprocmask(SIG_UNBLOCK, &sset, NULL);
		return NULL;

	} else if (status == CLD_TRAPPED) {
		printf("child %d got trapped\n", childpid);
		return NULL;
	} else if(status == CLD_DUMPED) {
		printf("child %d got dumped\n", childpid);
		return NULL;
	}
	return NULL;
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
	sigprocmask(SIG_BLOCK, &shellmask, NULL);

	sa.sa_flags = SA_SIGINFO;
	// sa.sa_sigaction = &sigchld_handler;
	sigaction(SIGCHLD, &sa, NULL);
	return TRUE;
}


/* ============================= update list ============================ */
int update_list(pid_t pid, int flag) {
	printf("in updating the job list\n");
	if(flag == terminated) {
		sem_wait(job);
		int result = dlist_remove_bypid(sus_bg_jobs, pid);
		sem_post(job);
		if(result == FALSE) {
			printf(" child %d is not in the 'job' list\n", pid);
			return FALSE;
		}
		return TRUE;
	}

	if(flag == fg_to_sus) {
		job_node* find = dlist_get_bypid(all_joblist, pid);
		if(find == NULL) {
			printf("yao si le mei zhao dao \n");
			return FALSE	;
		}
		job_node* target = jobnode_deepcopy(find);
		target->status = suspended;
		sem_wait(job);
		dlist_push_end(sus_bg_jobs, target);
		sem_post(job);
		return TRUE;
	}

	if(flag == bg_to_fg) {
		sem_wait(job);
		int result = dlist_remove_bypid(sus_bg_jobs, pid);
		sem_post(job);
		if(result == FALSE) {
			printf(" child %d is not in the 'job' list\n", pid);
			return FALSE;
		}
		return TRUE;
	}
	return TRUE;
}


/* ========================= for "jobs" command ========================= */
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


/* ========================== read and parse input ============================ */

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

	printf("4\n");
	job_node* jn = new_node(dlist_size(all_joblist)+1, NOTKNOWN, NOTKNOWN, NOTKNOWN, input, NULL, NULL);
	printf("5\n");
	jn->original_input = malloc(sizeof(char) * (strlen(input) + 1));
	sprintf(jn->original_input, input);
	jn->original_input[strlen(input)] = '\0';
	dlist_push_end(all_joblist, jn);
  return input;
}

parse_output* parse_input(char* input, char* delim) {
	char* cur = input;
	s.total = 0;
	int size = BUFSIZE;
	parse_output* po = malloc(sizeof(struct parse_output));
	if(po == NULL) {
		printf("malloc failed\n");
		return NULL;
	}
	struct tokenizer* t = init_tokenizer(input, delim);
	char* token = get_next_token(t);
	po->tasks = (char**)malloc(sizeof(char*) * size);
	if(token == NULL) {
		int len = strlen(input);
		if((po->tasks[total] = malloc(sizeof(char) *  len)) == NULL) {
			printf("malloc failed\n");
			return NULL;
		}
		sprintf(po->tasks[total], input);
		po->tasks[total][len] = '\0';
		po->num = 1;
		return po;
	} else if (strcmp(token, "\n") == 0) {
		int len = strlen(input) - 1;
		if(len == 0) {
			free(po->tasks);
			free(po);
			return NULL;
		}
		if((po->tasks[total] = malloc(sizeof(char) *  strlen(input))) == NULL) {
			printf("malloc failed\n");
			return NULL;
		}
		sprintf(po->tasks[total], input);
		po->tasks[total][len] = '\0';
		po->num = 1;
		return po;
	}

	while(token != NULL && strcmp(token, "\n") != 0) {
		int strlength = strlen(cur) - strlen(token);
		int malloclength = strlength + 1;
		if(token[strlen(token) - 1] == '&') { //if the current one is background job
			malloclength ++;
		}
		po->tasks[total] = malloc(sizeof(char) *  malloclength);
		sprintf(po->tasks[total], input);
		if(token[strlen(token) - 1] == '&') {
				po->tasks[total] += '&';
		}
		po->tasks[total][malloclength - 1] = '\0';

		s.total += 1;
		if(s.total >= BUFSIZE) {
			size += BUFSIZE;
			po->tasks = (char**)realloc(po->tasks, size);
		}
		token = get_next_token(t);
	}
	po->num = total + 1;
	return po;
}


/* ============================== executions =============================== */

int execute(char* task) {
	int bg = FALSE;
	printf("task in execute is %s\n", task);
	parse_output* jobs = parse_input(task, " ");
	parse_output* bgjob = parse_input(task, "&");

	if(bgjob->num == 1 && bgjob->tasks[0][strlen(bgjob->tasks[0]) - 1] == '&') {
		bg = TRUE;
	} else if(bgjob->num  > 1) {
		bg = TRUE;
	}
	// !!!!! free bgjob
	printf("current job is %d background or not\n", bg);

	if(jobs->num == 0) {
		printf("No input\n");
		return TRUE;
	}

	pid_t pid = fork();
	if(pid < 0) {
		perror("Fork failed \n");
		return TRUE;
	}
	else if(pid == 0) {
		pid_t chpid = getpid();
		pid_t chgid;
		int status = bg == TRUE ? background : foreground ;
		job_node* newjob = new_node(dlist_size(all_joblist) + 1, status, chpid, NOTKNOWN, task, NULL, NULL);
		if(setpgid(chpid, chpid) < 0 ) {
			perror("set child gid failed: ");
		}
		chgid = getpgid(getpid());
		newjob->gpid = chgid;

		/* no idea what is this
		if (infile != STDIN_FILENO)
    {
      dup2 (infile, STDIN_FILENO);
      close (infile);
    }
  if (outfile != STDOUT_FILENO)
    {
      dup2 (outfile, STDOUT_FILENO);
      close (outfile);
    }
  if (errfile != STDERR_FILENO)
    {
      dup2 (errfile, STDERR_FILENO);
      close (errfile);
    }
*/
		// unblock signals for childpid
		signal (SIGINT, SIG_DFL);
		signal (SIGQUIT, SIG_DFL);
		signal (SIGTSTP, SIG_DFL);
		signal (SIGTTIN, SIG_DFL);
		signal (SIGTTOU, SIG_DFL);
		signal (SIGTERM, SIG_DFL);

		tcgetattr(STDOUT_FILENO, &newjob->jmode);
		dlist_push_end(all_joblist, newjob);

		if(bg) {

			job_node* njobcpy = jobnode_deepcopy(newjob);
			njobcpy->index = dlist_size(sus_bg_jobs) + 1;
			dlist_push_end(sus_bg_jobs, njobcpy);

			if(execvp(jobs->tasks[0], jobs->tasks) < 0) {
				perror("Execution errror: ");
			}
			// free this jobs
			exit(0);

		} else {
		  printf("in here %s execution\n", jobs->tasks[0]);
		  tcsetattr(mysh_fd, TCSANOW, &newjob->jmode);
		  tcsetpgrp(mysh_fd, chgid);

			if(execvp(jobs->tasks[0], jobs->tasks) < 0) {
				perror("Execution errror: ");
			}

			//free the jobs
			//free(jobargs);
			exit(0);

		}

	} else if(pid > 0) {
		int stat;
		if(bg) {
			waitpid(pid, &stat, WNOHANG);
		} else {
		  tcsetpgrp(STDIN_FILENO, pid);
			waitpid(pid, &stat, WUNTRACED);
			if(WIFSTOPPED(stat)){
				update_list(pid, fg_to_sus);
				tcsetpgrp(mysh_fd, getpgid(getpid()));
				tcsetattr(mysh_fd, TCSANOW, &mysh);
			}
		}
	}
	return TRUE;
}


int execute_input(char* task) {
	int result = TRUE;
	parse_output* p = parse_input(task, " ");
	printf("current task is %s\n", p->tasks[0]);
	if(strcmp(p->tasks[0], "jobs") == 0) {
		print_jobs(sus_bg_jobs);
		// free processes
	} else if(strcmp(p->tasks[0], "bg") == 0) {
		printf("to be implemented\n");
	} else if(strcmp(p->tasks[0], "fg") == 0) {
		printf("to be implemented\n");
	} else if (strcmp(p->tasks[0], "kill") == 0) {
		printf("to be implemented\n");
	} else if(strcmp(p->tasks[0], "exit") == 0) {
		// need to free p
		result = FALSE;
	}else {

		printf("not yet\n");
		// after fork needs to store the termios immediately
		execute(task);
	}
	return result;
}


/* ============================ clean up stuff ============================= */
void free_joblists() {
	dlist_free(sus_bg_jobs);
	dlist_free(all_joblist);
}

void free_parser(parse_output* po) {
	if(po != NULL) {
		if(po->num > 0) {
			int index = 0;
			for(int i = 0; i < po->num; i++) {
				free(po->tasks[index]);
			}
		}
		free(po->tasks);
	}
	free(po);
}

int main(int argc, char* argv[]){
	// sets up
  set_up_signals();
	int run = FALSE;
	shell_pid = getpid();
	if(setpgid(shell_pid, shell_pid) < 0) {
		perror("Reset shell gpid failed\n");
		exit(FALSE);
	}
	shell_gpid = getpgid(shell_pid);
	if(shell_gpid != tcgetpgrp(mysh_fd)) {
		int result = tcsetpgrp(mysh_fd, shell_gpid);
		if(result < 0) {
			perror("Setting shell to foreground failed\n");
		}
	}
	init_sems();
	init_joblists();

	tcgetattr(mysh_fd, &mysh);

	do {
		// starts executing
		// check if need to store the shell termios here
		char* input = read_input();
		if(input == NULL) {
			printf("No input \n");
			run = TRUE;
			continue;
		}
		parse_output* nonewline = parse_input(input, "\n");
		if(nonewline == NULL) {
			printf("No input \n");
			run = TRUE;
			continue;
		}
		parse_output* job = parse_input(nonewline->tasks[0], ";");
		for (int i = 0; i < job->num; i++) {
			parse_output* p = parse_input(job->tasks[i], "&");
			for(int j = 0; j < p->num; j++) {
				run = execute_input(p->tasks[0]);
			}
			// free curjob
		}
		//printf("before free input 443\n");
		free(input);
		// check if need to restore the shell termios here
		// free multijobs
	} while (run);
	close_sems();
	// clean up everything
}

void* create_shared_memory(size_t size){
  int protection = PROT_READ | PROT_WRITE;
  int visibility = MAP_ANONYMOUS | MAP_SHARED;
  return mmap(NULL, size, protection, visibility, 0, 0);
}
// int check_special_symbols(char* input) {
// 	execjob_num = 0;
// 	struct tokenizer* t = init_tokenizer(input, special_delim);
// 	char* token = get_next_token(t);
// 	while(token != NULL) {
// 		execjob_num += 1;
// 		token = get_next_token(t);
// 	}
// 	free_tokenizer(t);
// 	return execjob_num;
// }


// void test_job_list(){
//   dlist dl = dlist_new();
//   job_node* j1 = new_node(1, 0,0,0,"first job",NULL, NULL, mysh);
//   dlist_push_end(dl, j1);
//   job_node* j2 = new_node(2,0,1,0,"second job", NULL, NULL, mysh);
//   insert_after(j1, j2);
//   job_node* j3 = new_node(3,0,2,0,"thrid job", NULL, NULL, mysh);
//   insert_after(j2,j3);
//   job_node* j4 = new_node(4,0,3,0, "fourth job", NULL,NULL, mysh);
//   insert_after(j3, j4);
//   job_node* j5 = new_node(5,0,4,0, "5th job", NULL,NULL, mysh);
//   insert_after(j4, j5);
//   job_node* j12 = new_node(0,0,5,0,"after 1st before 2nd", NULL,NULL, mysh);
//   job_node* h = dl->head;
//   dlist_push_end(dl, j2);
// 	dlist_push_end(dl, j3);
// 	// dlist_remove(dl, 1);
//
//   // printf("%s\n",(dl->tail)->original_input );
//   while (h){
//     dlist_push_end(dl, h);
//     printf(h->original_input);
//     printf("\n" );
//     h = h->next;
//   }
//   printf("%d\n",dlist_size(dl) );
//   dlist_insert(dl,1, j12);
//   dlist_remove(dl, 2);
//   h = j1;
//   // while (h){
//   //   printf(h->original_input);
//   //   printf("\n" );
//   //   h = h->next;
//   // }
//   delete_node(j1);
// }


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
