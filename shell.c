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
#include <ctype.h>

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
#define TOBG "bg"
#define TOFG "fg"
#define KILL "kill"
#define JOBS "jobs"
#define EXIT "exit"

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


/* ======================= Initializing Jobs ============================ */
void init_joblists() {
  job_num = 0;
  all_joblist = dlist_new();
  sus_bg_jobs = dlist_new();
}


/* ========================== Handle Signals ============================== */
void sigchld_handler(int signal, siginfo_t* sg, void* oldact) {
  pid_t childpid = sg->si_pid;
  int status = sg->si_code;
  if(status == CLD_EXITED) {
    printf("sighandler: child terminated\n");
    update_list(childpid, terminated);
    return;
  } else if (status == CLD_KILLED) {
    update_list(childpid, terminated);
    return;
  } else if (status == CLD_STOPPED) {
    printf("sighanlder: child stopped\n");
    update_list(childpid, fg_to_sus);
    return;
  } else if (status == CLD_CONTINUED) {
    update_list(childpid, bg_to_fg);
    return;
  } else if (status == CLD_TRAPPED) {
    printf("child %d got trapped\n", childpid);
    return;
  } else if(status == CLD_DUMPED) {
    printf("child %d got dumped\n", childpid);
    return;
  } else {
    printf("the status in signal handler is %d\n", status);
  }
  return;
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

  sa.sa_flags = SA_SIGINFO | SA_RESTART;
 	sa.sa_sigaction = sigchld_handler;
  sigaction(SIGCHLD, &sa, NULL);
  return TRUE;
}


/* ============================= update list ============================ */
int update_list(pid_t pid, int flag) {
  sigset_t sset;
  sigaddset(&sset, SIGCHLD);
  sigprocmask(SIG_BLOCK, &sset, NULL);

  printf("in updating the job list\n");

	if(flag == terminated) {
		printf("update_list: removing child\n");
    int result = dlist_remove_bypid(sus_bg_jobs, pid);
		printf("update_list: child removed\n");
    sigprocmask(SIG_UNBLOCK, &sset, NULL);
    if(result == FALSE) {
      printf(" child %d is not in the 'job' list\n", pid);
      return FALSE;
    }
    return TRUE;
  }

  if(flag == bg_to_fg) {
    int result = dlist_remove_bypid(sus_bg_jobs, pid);
    sigprocmask(SIG_UNBLOCK, &sset, NULL);
    if(result == FALSE) {
      printf(" child %d is not in the 'job' list\n", pid);
      return FALSE;
    }
    return TRUE;
  }

	/*if(flag == fg_to_sus) {
    job_node* find = dlist_get_bypid(all_joblist, pid);
    if(find == NULL) {
      printf("yao si le mei zhao dao \n");
      return FALSE;
    }
    job_node* target = jobnode_deepcopy(find);
    target->status = suspended;
    dlist_push_end(sus_bg_jobs, target);
    sigprocmask(SIG_UNBLOCK, &sset, NULL);
    return TRUE;
  }*/

  return TRUE;
}

/* =============================== read in ==================================== */
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

  job_node* jn = new_node(dlist_size(all_joblist)+1, NOTKNOWN, NOTKNOWN, NOTKNOWN, input, NULL, NULL);
  jn->original_input = malloc(sizeof(char) * (strlen(input) + 1));
  sprintf(jn->original_input, input);
  dlist_push_end(all_joblist, jn);
  return input;
}

/* ========================= for "jobs" command ========================= */
void print_jobs(dlist jobs) {
	int length = 15;
  job_node* top = get_head(jobs);
  if(jobs == NULL) {
    printf("No jobs available.\n");
  } else if (top == NULL) {
    printf("No jobs yet. \n");
  }

  while(top != NULL) {
		char* status = malloc(sizeof(char) * length);
		switch(top->status) {
			case background:
				strcpy(status, "background");
				break;
			case foreground:
				strcpy(status, "foreground");
				break;
			case suspended:
				strcpy(status, "stopped");
				break;
		}
    printf("%d. %s	%s\n", top->index, get_input(top), status);
    top = top->next;
		free(status);
  }
}


/* ============================== bring to fg, bg, and kill =============================== */

int bring_tobg(parse_output* p) {
	sigset_t sset;
	sigaddset(&sset, SIGCHLD);
	int result = TRUE;
	int no_arg = 1;
	int job_index; // index for the job backwards

	if(p->num == no_arg) { // when bg has no argument
		job_index = 1;

		job_node* job = dlist_get(sus_bg_jobs, job_index);
		if(job != NULL) {
			if(job->status == suspended) {
				int killresult = kill(job->gpid, SIGCONT);
				if(killresult == 0) {
				  printf("bring+bg: signal successfully sent\n");
				  sigprocmask(SIG_BLOCK, &sset, NULL);
					job->status = background;
					sigprocmask(SIG_UNBLOCK, &sset, NULL);
					result = TRUE;
				} else {
					printf("Sending SIGCONT to process group %d failed!\n", job->gpid);
				}
			} else if(job->status == background) {
				printf("Process group %d already in background\n", job->gpid);
			}
		} else {
			printf("Process group %d not in job list\n", job_index);
		}

	} else {
		for(int i = 1; i < p->num; i++) {
			job_index = to_int(p->tasks[i]);
			printf(" in bringing to background job index is %d\n", job_index);
			if(job_index != 0) {

				job_node* job = dlist_get(sus_bg_jobs,job_index);
				if(job != NULL) {
					if(job->status == suspended) {
						int killresult = kill(job->gpid, SIGCONT);
						if(killresult == 0) {
							sigprocmask(SIG_BLOCK, &sset, NULL);
							job->status = background;
							sigprocmask(SIG_UNBLOCK, &sset, NULL);
							result = TRUE;
						} else {
							printf("Sending SIGCONT to process group %d failed!\n", job->gpid);
						}
					} else if(job->status == background) {
						printf("Process group %d already in background\n", job->gpid);
					}
				} else {
					printf("Process group %d not in job list\n", job_index);
				}

			} else {
				printf("in valid index %d\n", job_index);
			}
		}
	}
	return result;
}


int bring_tofg(parse_output* p) {
	sigset_t sset;
	sigaddset(&sset, SIGCHLD);
	int result = TRUE;
	int no_arg = 1;
	int toint;
	job_node* job;

	if(p->num == no_arg) { // when bg has no argument
		toint = 1;
		job = dlist_get(sus_bg_jobs, toint);
	} else {
		for(int i = 1; i < p->num; i++) {
			toint = to_int(p->tasks[i]);
			printf("tofg: in bringing to foreground job index is %d\n", toint);
			if(toint != FALSE) {
				job_node* job = dlist_get(sus_bg_jobs, toint);
				if(job == NULL) {
					continue;
				}
			}
		}
	}

	if(job == NULL) {
		printf("Process group %d not in job list\n", toint);
		return TRUE;
	}


	if (job->status == suspended) {

		int killresult = kill(job->gpid, SIGCONT);
		if(killresult == 0) {
			sigprocmask(SIG_BLOCK, &sset, NULL);
			job->status = background;
			sigprocmask(SIG_UNBLOCK, &sset, NULL);
		}
	}
	if(job->status == background) {
		printf("bringfg: child status is background\n");
		int setgrp = tcsetpgrp(mysh_fd, job->gpid);
		if(setgrp == SUCCESS) {
		  printf("tobg: setting terminal control\n");
			int setattr = tcsetattr(mysh_fd, TCSADRAIN, &(job->jmode));
			if(setattr == SUCCESS) {
			  
			  printf("tobg: setting terminal control success\n");
			  int stat;
				int oldpid = job->pid;
				int oldgid = job->gpid;
				char* oldinput = (char*)malloc(sizeof(char) * (strlen(job->original_input) + 1));
				sprintf(oldinput, "%-*s", (int)strlen(job->original_input), job->original_input);
				
				sigprocmask(SIG_BLOCK, &sset, NULL);
				dlist_remove_bypid(sus_bg_jobs, oldpid);
				sigprocmask(SIG_UNBLOCK, &sset, NULL);
				waitpid(job->pid, &stat, WUNTRACED);
				if(WIFSTOPPED(stat)) {
					struct termios childt;
					tcgetattr(STDOUT_FILENO, &childt);
					sigprocmask(SIG_BLOCK, &sset, NULL);
					job_node* newjob = new_node(dlist_size(sus_bg_jobs) + 1, suspended, oldpid, oldgid, oldinput, NULL, NULL);
					newjob->jmode = childt;
					dlist_push_end(sus_bg_jobs, newjob);
					sigprocmask(SIG_UNBLOCK, &sset, NULL);
				} else {
					free(oldinput);
				}
				tcsetpgrp(mysh_fd, shell_gpid);
				tcsetattr(mysh_fd, TCSADRAIN, &mysh);
			} else {
				printf("Set process group with index %d to terminal failed\n", toint);
				tcsetpgrp(mysh_fd, shell_gpid);
			}
		} else {
			printf("Set process group with index %d to foreground failed\n", toint);
		}
		return TRUE;

	}
	return result;
}

int kill_process(parse_output* p) {
	int no_arg = 1;
	int is_sigkill = 3;
	int is_sigterm = 2;
	int flag_index = 1;
	sigset_t sset;
	sigaddset(&sset, SIGCHLD);
	for(int i = 0; i < p->num; i++) {
	  printf("kill: %d. %s\n", i, p->tasks[i]); 
	}
	if(p->num == no_arg) {
	  printf("kill: no_ard\n");
		printf("kill: usage: kill -flag(optional) job_index(integer)\n");
		return TRUE;
	} else {
		int sigkill = (strcmp(p->tasks[flag_index], "-9") == 0);
		if(sigkill) {
			if(p->num != is_sigkill) {
			  printf("kill: is_sigkill wrong syntax in -9\n");
				printf("kill: usage: kill -flag(optional) job_index(integer)\n");
				return TRUE;
			}
		} else {
			if(p->num != is_sigterm) {
			  printf("kill: is_sigterm wrong tasks->num\n");
				printf("kill: usage: kill -flag(optional) job_index(integer)\n");
				return TRUE;
			}
		}
		int indexnum = sigkill ? 2 : 1;
		if((char)p->tasks[indexnum][0] != '%') {
			printf("kill: Operating not permitted\n");
			return TRUE;
		} else {
			int job_index = to_int(p->tasks[indexnum]);
			if(job_index == FALSE) {
			  printf("kill: job index wrong format\n");
				printf("kill: usage: kill -flag(optional) job_index(integer)\n");
				return TRUE;
			} else {
				job_node* job = dlist_get(sus_bg_jobs, job_index);
				if(job == NULL) {
					printf("kill: no such job");
					return TRUE;
				} else {
					int kill_signal = sigkill ? SIGKILL : SIGTERM;
					printf("kill: the signal is %d\n", kill_signal == SIGTERM);
					int kill_result = kill(job->gpid, kill_signal);
					if(kill_result != 0) {
						printf("kill: SIGKILL terminates process group with index %d failed\n", job_index);
					} else {
					  	sigprocmask(SIG_BLOCK, &sset, NULL);
						dlist_remove_bypid(sus_bg_jobs, job->gpid);
						sigprocmask(SIG_UNBLOCK, &sset, NULL);
			
					}
					return TRUE;
				}
			}
		}
	}
	return TRUE;
}

/* ============================== executions =============================== */
int execute_bg(char* task) {
  int result = TRUE;
  parse_output* p = parse_input(task, " ");
  struct termios term;
  sigset_t sset;
  sigaddset(&sset, SIGCHLD);
  job_node* newjob = new_node(dlist_size(sus_bg_jobs) + 1, background, NOTKNOWN, NOTKNOWN, task, NULL, NULL);
  sigprocmask(SIG_BLOCK, &sset, NULL);
  dlist_push_end(sus_bg_jobs, newjob);
  sigprocmask(SIG_UNBLOCK, &sset, NULL);
	
	if(p->num == 0) {
		printf("Invalid Input\n");
		return TRUE;
	}
  printf("current task is %s in background bbbbbbbbbbbbbbb\n", p->tasks[0]);
	if(strcmp(p->tasks[0], JOBS) == 0) {
		print_jobs(sus_bg_jobs);
		result = TRUE;
	} else if(strcmp(p->tasks[0], TOBG) == 0) {
		result = bring_tobg(p);
	} else if(strcmp(p->tasks[0], TOFG) == 0) {
		result = bring_tofg(p);
	} else if (strcmp(p->tasks[0], KILL) == 0) {
		kill_process(p);
	} else if(strcmp(p->tasks[0], EXIT) == 0) {
		// need to free p
		result = FALSE;
	}else {

		pid_t pid = fork();
		if(pid < 0) {
			perror("Fork failed: ");
			return TRUE;
		} else if (pid == 0) {
			// in child
			pid_t chpid = getpid();
			if(setpgid(chpid, chpid) < 0 ) {
	      perror("set child gid failed: ");
	    }

			signal (SIGINT, SIG_DFL);
			signal (SIGQUIT, SIG_DFL);
			signal (SIGTSTP, SIG_DFL);
			signal (SIGTTIN, SIG_DFL);
			signal (SIGTTOU, SIG_DFL);
			signal (SIGTERM, SIG_DFL);

			//need free(p)
			if(execvp(p->tasks[0], p->tasks) < 0) {
				perror("Execution error ");
				// free p;
				result = TRUE;
			}
			//			return result;
			exit(0);
		} else if (pid > 0) {
		  int stat;
		  tcgetattr(mysh_fd, &term);
 			sigprocmask(SIG_BLOCK, &sset, NULL);
			newjob->gpid = getpgid(pid);
			newjob->jmode = term;
			sigprocmask(SIG_UNBLOCK, &sset, NULL);
			waitpid(pid, &stat, WNOHANG);
		}
		tcsetpgrp(mysh_fd, getpgid(getpid()));
		tcsetattr(mysh_fd, TCSADRAIN, &mysh);
	}
	return result;
}



int execute_fg(char* task) {
  int result = TRUE;
  //tcgetattr(mysh_fd, &mysh);
  parse_output* p = parse_input(task, " ");
	if(p->num == 0) {
		printf("Invalid Input\n");
		return TRUE;
	}
  printf("current task is %s in foreground ffffffffffffffffffffffffffff\n", p->tasks[0]);

	if(strcmp(p->tasks[0], JOBS) == 0) {
		print_jobs(sus_bg_jobs);
		result = TRUE;
	} else if(strcmp(p->tasks[0], TOBG) == 0) {
		result = bring_tobg(p);
		result = TRUE;
	} else if(strcmp(p->tasks[0], TOFG) == 0) {
		result = bring_tofg(p);
	} else if (strcmp(p->tasks[0], KILL) == 0) {
		kill_process(p);
	} else if(strcmp(p->tasks[0], EXIT) == 0) {
		// need to free p
		result = FALSE;
	}else {
		printf("not yet\n");
		pid_t pid = fork();
		if(pid < 0) {
			perror("Fork failed: ");
			return TRUE;
		} else if (pid == 0) {
			// in child
			pid_t chpid = getpid();
			if(setpgid(chpid, chpid) < 0 ) {
	      perror("set child gid failed: ");
	    }

			signal (SIGINT, SIG_DFL);
			signal (SIGQUIT, SIG_DFL);
			signal (SIGTSTP, SIG_DFL);
			signal (SIGTTIN, SIG_DFL);
			signal (SIGTTOU, SIG_DFL);
			signal (SIGTERM, SIG_DFL);

			tcsetpgrp(mysh_fd, getpgid(pid));
			if(execvp(p->tasks[0], p->tasks) < 0) {
				perror("Execution error ");
				// free p;
				result = TRUE;
			}
			//		return result;
			//need free(p)
			exit(0);
		} else if (pid > 0) {
			int stat;
			sigset_t sset;
			sigaddset(&sset, SIGCHLD);
			//tcsetpgrp(mysh_fd, getpgid(pid));
			waitpid(pid, &stat, WUNTRACED);
			printf("in patent\n");
			if(WIFSTOPPED(stat)){
				printf("execute_fg: child stopped\n");
				struct termios childt;
				tcgetattr(STDOUT_FILENO, &childt);
				sigprocmask(SIG_BLOCK, &sset, NULL);
				job_node* newjob = new_node(dlist_size(sus_bg_jobs) + 1, suspended, pid,  getpgid(pid), task, NULL, NULL);
				newjob->jmode = childt;
				dlist_push_end(sus_bg_jobs, newjob);
				sigprocmask(SIG_UNBLOCK, &sset, NULL);
			}
			tcsetpgrp(mysh_fd, shell_gpid);
			tcsetattr(mysh_fd, TCSADRAIN, &mysh);
		}
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

/* =========================== useful function =========================== */

int to_int(char* str) {
	int len = strlen(str);
	int result = FALSE;
	for(int i = 0; i < len; i ++) {
		if(isdigit(str[i]) != 0) {
			result *= 10;
			result += str[i] - '0';
		}
	}
	return result;
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
  init_joblists();
  tcgetattr(mysh_fd, &mysh);

  do {
		char* input = read_input();
		printf("1a\n");
		parse_output* newline = parse_input(input, "\n");
		if(newline->num == 1) {
			run = TRUE;
			// free newline
			continue;
		}
		parse_output* jobs = parse_input(newline->tasks[0], ";");
		if(jobs->num == 1 && strcmp(newline->tasks[0], ";") == 0) {
			run = TRUE;
			printf("Invalid input\n");
			// free jobs
			// free newline
			continue;
		}
		int symbolnum = 0;
		printf("2a\n");
		for(int i = 0; i < jobs->num; i++) {
			if(strcmp(jobs->tasks[i], ";") == 0) {
				jobs->tasks[i] = NULL;
				symbolnum ++;
			}
		}

		printf("3a\n");
		int jobnum = jobs->num - symbolnum;
		char* job = NULL;
		for(int i = 0; i < jobs->num; i++) {
			job = jobs->tasks[i];
			if(job != NULL) {
				break;
			}
		}
		printf("4a job num is %d\n", jobnum);
		for(int i = 0; i < jobnum; i++) {
			printf("Main: in smalljob\n");
			parse_output* smalljob = parse_input(job, "&");
			for(int j = 0; j < smalljob->num; j++) {
				if(strcmp(smalljob->tasks[j], "&") != 0) {
					if(j == smalljob->num - 1) {
						run = execute_fg(smalljob->tasks[j]);
					} else if(strcmp(smalljob->tasks[j+1], "&") == 0) {
						run = execute_bg(smalljob->tasks[j]);
					}
				} else {
					continue;
				}
			}
			// free smalljob
			job += 2;
		}
		// need to free newline, jobs
		free(input);
  } while (run);
  // clean up everything
}









/*
int execute(char* task) {
  int bg = FALSE;
  printf("task in execute is %s", task);
  parse_output* jobs = parse_input(task, " ");
  parse_output* bgjob = parse_input(task, "&");

  if(bgjob->num == 1 && bgjob->tasks[0][strlen(bgjob->tasks[0]) - 1] == '&') {
    bg = TRUE;
		char temp[strlen(bgjob->tasks[0])];
		sprintf(temp, bgjob->tasks[0]);
		sprintf(bgjob->tasks[0], "%-*s", (int)strlen(bgjob->tasks[0]) - 1, bgjob->tasks[0]);
		bgjob->tasks[0][(int)(strlen(bgjob->tasks[0]) - 1)] = '\0';
  } else if(bgjob->num  > 1) {
		sprintf(bgjob->tasks[0], "%-*s", (int)strlen(bgjob->tasks[0]) - 1, bgjob->tasks[0]);
		bgjob->tasks[0][(int)(strlen(bgjob->tasks[0]) - 1)] = '\0';
    bg = TRUE;
  }
  // !!!!! free bgjob
  printf("current job is %d background %s.  \n", bg, bgjob->tasks[0]);

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
    if(setpgid(chpid, chpid) < 0 ) {
      perror("set child gid failed: ");
    }
     no idea what is this
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

    // unblock signals for childpid
    signal (SIGINT, SIG_DFL);
    signal (SIGQUIT, SIG_DFL);
    signal (SIGTSTP, SIG_DFL);
    signal (SIGTTIN, SIG_DFL);
    signal (SIGTTOU, SIG_DFL);
    signal (SIGTERM, SIG_DFL);

    if(execvp(jobs->tasks[0], jobs->tasks) < 0) {
      perror("Execution error ");
			exit(0);
			return TRUE;
    }
    // free this jobs
    exit(0);

  } else if(pid > 0) {
    int stat;
		sigset_t sset;
		sigaddset(&sset, SIGCHLD);
    if(bg) {
			printf("parent is creating\n");
      job_node* newjob = new_node(dlist_size(sus_bg_jobs) + 1, background, pid, NOTKNOWN, task, NULL, NULL);
			printf("parent finish creating\n");
			newjob->gpid = getpgid(pid);
			sigprocmask(SIG_BLOCK, &sset, NULL);
			dlist_push_end(sus_bg_jobs, newjob);
			sigprocmask(SIG_UNBLOCK, &sset, NULL);
			printf("parent finish adding\n");
      waitpid(pid, &stat, WNOHANG);
    } else {
      //tcsetpgrp(STDIN_FILENO, pid);
      waitpid(pid, &stat, WUNTRACED);
      printf("in patent\n");
      if(WIFSTOPPED(stat)){
				struct termios childt;
				tcgetattr(STDOUT_FILENO, &childt);
				job_node* newjob = new_node(dlist_size(sus_bg_jobs) + 1, suspended, pid,  getpgid(pid), task, NULL, NULL);
				newjob->jmode = childt;
				sigprocmask(SIG_BLOCK, &sset, NULL);
				dlist_push_end(sus_bg_jobs, newjob);
				sigprocmask(SIG_UNBLOCK, &sset, NULL);
      }
      tcsetpgrp(mysh_fd, getpgid(getpid()));
      tcsetattr(mysh_fd, TCSADRAIN, &mysh);
    }
  }
  return TRUE;
}



*/




/*
do {
	// starts executing
	// check if need to store the shell termios here
	char* input = read_input();
	printf("the input in main is %s", input);
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
	printf("the input in main after job parse is %s", job->tasks[0]);
	for (int i = 0; i < job->num; i++) {
		parse_output* p = parse_input(job->tasks[i], "&");
		printf("the input in main after p parse is %s  num %d", p->tasks[0], p->num);
		for(int j = 0; j < p->num; j++) {
			printf("start running in main\n");
			run = execute_input(p->tasks[0]);
		}
		// free curjob
	}
	//printf("before free input 443\n");
	free(input);
	// check if need to restore the shell termios here
	// free multijobs
} while (run);
*/



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
