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
#define FAILURE -1
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

// job lists
dlist sus_bg_jobs;

// globals
// terminal attribute related globals
pid_t shell_pid;
pid_t shell_gpid;
struct termios mysh;
int mysh_fd = STDIN_FILENO;
struct sigaction sa;


/* ======================= Initializing Job List ============================ */
void init_joblists() {
  sus_bg_jobs = dlist_new();
}

/* ========================== Handle Signals ============================== */

/*
  Signal handler for SIGCHLD
*/
void sigchld_handler(int signal, siginfo_t* sg, void* oldact) {
  pid_t childpid = sg->si_pid;
  int status = sg->si_code;
  if(status == CLD_EXITED) {
    update_list(childpid, terminated);
    return;
  } else if (status == CLD_KILLED) {
    update_list(childpid, terminated);
    return;
  } else if (status == CLD_STOPPED) {
    update_list(childpid, fg_to_sus);
    return;
  } else if (status == CLD_CONTINUED) {
    return;
  } else if (status == CLD_TRAPPED) {
    return;
  } else if(status == CLD_DUMPED) {
    return;
  }
  return;
}

/*
  set up signal mask and register signal handler
*/
int set_up_signals() {
  sigset_t shellmask;

  sigemptyset(&shellmask);
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
/*
  update background and stopped job list
*/
int update_list(pid_t pid, int flag) {
  sigset_t sset;
  sigemptyset(&sset);
  sigaddset(&sset, SIGCHLD);
  sigprocmask(SIG_BLOCK, &sset, NULL);

  if(flag == terminated) {
    int result = dlist_remove_bypid(sus_bg_jobs, pid);
    sigprocmask(SIG_UNBLOCK, &sset, NULL);
    if(result == FALSE) {
      return FALSE;
    }
    return TRUE;
  }

  if(flag == bg_to_fg) {
    int result = dlist_remove_bypid(sus_bg_jobs, pid);
    sigprocmask(SIG_UNBLOCK, &sset, NULL);
    if(result == FALSE) {
      return FALSE;
    }
    return TRUE;
  }

  if(flag == fg_to_sus) {
    job_node* find = dlist_get_bypid(sus_bg_jobs, pid);
    if(find == NULL) {
      sigprocmask(SIG_UNBLOCK, &sset, NULL);
      return FALSE;
    } else {
      find->status = suspended;
    }
    sigprocmask(SIG_UNBLOCK, &sset, NULL);
    return TRUE;
  }

  return TRUE;
}

/* =============================== read in ==================================== */
/*
  read in the input and add one jobnode(with original input)
*/
char* read_input() {
  size_t readn;
  char* input = NULL;
  printf("mysh > ");
  if(getline(&input, &readn, stdin) < 0) {
    printf("getline from stdin failed! \n");
    free(input);
    input = NULL;
    return input;
  }
  return input;
}

/* ========================= for "jobs" command ========================= */
/*
  execute "jobs" command
*/
void print_jobs(dlist jobs) {
  int length = 15;
  job_node* top = get_head(jobs);
  if(jobs == NULL) {
    printf("No jobs available.\n");
  } else if (top == NULL) {
    printf("No jobs available.\n");
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
    printf("%d. pid: %d  pgid: %d	%s	%s\n", top->index, top->pid, top->gpid, get_input(top), status);
    top = top->next;
    free(status);
  }
}


/* ============================== bring to fg, bg, and kill =============================== */
/*
  bringing a process to background
*/
int bring_tobg(parse_output* p) {
  sigset_t sset;
  sigemptyset(&sset);
  sigaddset(&sset, SIGCHLD);
  int result = TRUE;
  int no_arg = 1;
  int job_index;
  job_node* job;

  if(p->num == no_arg) { // when bg has no argument
    job_index = 1;
    job = dlist_get(sus_bg_jobs, job_index);
    if(job != NULL) {
      if(job->status == suspended) {
	int killresult = kill(job->gpid, SIGCONT);
	if(killresult == 0) {
	  sigprocmask(SIG_BLOCK, &sset, NULL);
	  job->status = background;
	  sigprocmask(SIG_UNBLOCK, &sset, NULL);
	  result = TRUE;
	} else {
	  perror("Sending SIGCONT to process group failed: ");
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
      if(job_index != 0) {

	job = dlist_get(sus_bg_jobs,job_index);
	if(job != NULL) {
	  if(job->status == suspended) {
	    int killresult = kill(job->gpid, SIGCONT);
	    if(killresult == 0) {
	      sigprocmask(SIG_BLOCK, &sset, NULL);
	      job->status = background;
	      sigprocmask(SIG_UNBLOCK, &sset, NULL);
	      result = TRUE;
	    } else {
	      perror("Sending SIGCONT in bring to background: ");
	    }
	  } else if(job->status == background) {
	    printf("Process %d already in background\n", job->gpid);
	  }
	} else {
	  printf("Process %d not in job list\n", job_index);
	}

      } else {
	printf("Invalid index %d\n", job_index);
      }
    }
  }
  return result;
}

/*
  - execute 'fg %<#>' commands
  - bring a process to foreground
  - if the process bringing to foreground is stopped, a SIGCONT is sent
*/
int bring_tofg(parse_output* p) {
  sigset_t sset;
  sigemptyset(&sset);
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
      if(toint != FALSE) {
	job = dlist_get(sus_bg_jobs, toint);
	if(job == NULL) {
	  continue;
	} else {
	  break;
	}
      }
    }
  }

  if(job == NULL) {
    printf("Process group %d not in job list\n", toint);
    return TRUE;
  }

  if (job->status == suspended) {
    if(kill(job->gpid, SIGCONT) == 0) {
      sigprocmask(SIG_BLOCK, &sset, NULL);
      job->status = background;
      sigprocmask(SIG_UNBLOCK, &sset, NULL);
    } else {
      perror("Failed to send signal: ");
      return TRUE;
    }
  }

  if(tcsetpgrp(mysh_fd, job->gpid) == FAILURE) {
    perror("bringfg: set process to fg failed: ");
    return TRUE;
  }
  if(tcsetattr(mysh_fd, TCSADRAIN, &job->jmode) != FAILURE) {
    int stat;
    int oldpid = job->pid;
    int oldgid = job->gpid;
    char* oldinput = (char*)malloc(sizeof(char) * (strlen(job->original_input) + 1));
    sprintf(oldinput, "%-*s", (int)strlen(job->original_input), job->original_input);

    sigprocmask(SIG_BLOCK, &sset, NULL);
    dlist_remove_bypid(sus_bg_jobs, oldpid);
    sigprocmask(SIG_UNBLOCK, &sset, NULL);
    waitpid(oldpid, &stat, WUNTRACED);
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
    perror("Setting process to foreground: ");
  }
  return result;
}


/*
  - command 'kill' for killing Process
  - does not allow user input job index without '%'
  - "-9" flag for sending SIGKILL
*/
int kill_process(parse_output* p) {
  int no_arg = 1;
  int is_sigkill = 3;
  int is_sigterm = 2;
  int flag_index = 1;
  sigset_t sset;
  sigemptyset(&sset);
  job_node* job;

  sigaddset(&sset, SIGCHLD);
  if(p->num == no_arg) {
    printf("kill: usage: kill -flag(optional) job_index(integer)\n");
    return TRUE;
  } else {
    int sigkill = (strcmp(p->tasks[flag_index], "-9") == 0);
    if(sigkill) {
      if(p->num != is_sigkill) {
	printf("kill: usage: kill -flag(optional) job_index(integer)\n");
	return TRUE;
      }
    } else {
      if(p->num != is_sigterm) {
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
	printf("kill: usage: kill -flag(optional) job_index(integer)\n");
	return TRUE;
      } else {
	job = dlist_get(sus_bg_jobs, job_index);
	if(job == NULL) {
	  printf("kill: no such job");
	  return TRUE;
	} else {
	  if(job->status == suspended) {
	    if(kill(job->gpid, SIGCONT) == FAILURE) {
	      perror("Continue job failed: ");
	    }
	  }
	  if(kill(job->gpid, sigkill ? SIGKILL : SIGTERM) == FAILURE) {
	    perror("kill: sending signal: ");
	  }
	  return TRUE;
	}
      }
    }
  }
  return TRUE;
}

/* ============================== executions =============================== */
/*
  launch a process in background
*/
int execute_bg(char* task) {
  int result = TRUE;
  struct termios term;
  sigset_t sset;

  job_node* newjob = new_node(dlist_size(sus_bg_jobs) + 1, background, NOTKNOWN, NOTKNOWN, task, NULL, NULL);
  sigemptyset(&sset);
  sigaddset(&sset, SIGCHLD);
  sigprocmask(SIG_BLOCK, &sset, NULL);
  dlist_push_end(sus_bg_jobs, newjob);
  sigprocmask(SIG_UNBLOCK, &sset, NULL);
  parse_output* p = parse_input(task, " ");

  if(p->num == 0) {
    printf("Invalid Input\n");
    //free_parse_output(p);
    return TRUE;
  }
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
    //free_parse_output(p);
    result = FALSE;
    return result;
  }else {

    pid_t pid = fork();
    if(pid < 0) {
      perror("Fork failed: ");
      //free_parse_output(p);
      result = TRUE;
      return result;
    } else if (pid == 0) {
      // in child
      pid_t chpid = getpid();
      if(setpgid(chpid, chpid) < 0 ) {
	perror("Set child gid failed: ");
	//free_parse_output(p);
	result = TRUE;
	exit(SUCCESS);
      }

      signal (SIGINT, SIG_DFL);
      signal (SIGQUIT, SIG_DFL);
      signal (SIGTSTP, SIG_DFL);
      signal (SIGTTIN, SIG_DFL);
      signal (SIGTTOU, SIG_DFL);
      signal (SIGTERM, SIG_DFL);

      if(execvp(p->tasks[0], p->tasks) < 0) {
	perror("Execution error ");
	//free_parse_output(p);
	result = TRUE;
      }
      exit(SUCCESS);
    } else if(pid > 0){
      int stat;
      setpgid(pid, pid);
      setpgid(getpid(), getpid());
      tcgetattr(mysh_fd, &term);
      sigprocmask(SIG_BLOCK, &sset, NULL);
      newjob->pid = pid;
      newjob->gpid = getpgid(pid);
      newjob->jmode = term;
      sigprocmask(SIG_UNBLOCK, &sset, NULL);
      waitpid(pid, &stat, WNOHANG);
      tcsetpgrp(mysh_fd, getpgid(getpid()));
      tcsetattr(mysh_fd, TCSADRAIN, &mysh);
      //free_parse_output(p);
    }
  }
  return result;
}


/*
  launch a process in foreground
*/
int execute_fg(char* task) {
  int result = TRUE;
  struct termios jterm;
  parse_output* p = parse_input(task, " ");
  int stat;
  sigset_t sset;
  sigemptyset(&sset);
  sigaddset(&sset, SIGCHLD);
		
  if(p->num == 0) {
    printf("Invalid Input\n");
    //free_parse_output(p);
    result = TRUE;
    return result;
  }

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
    //free_parse_output(p);
    result = FALSE;
    return result;
  }else {
    pid_t pid = fork();
    if(pid < 0) {
      perror("Fork failed: ");
      //free_parse_output(p);
      result = TRUE;
      return result;
    } else if (pid == 0) {
      pid_t chpid = getpid();
      if(setpgid(chpid, chpid) < 0 ) {
	perror("Set child gid failed: ");
	//free_parse_output(p);
	result = TRUE;
	exit(SUCCESS);
      }

      signal (SIGINT, SIG_DFL);
      signal (SIGQUIT, SIG_DFL);
      signal (SIGTSTP, SIG_DFL);
      signal (SIGTTIN, SIG_DFL);
      signal (SIGTTOU, SIG_DFL);
      signal (SIGTERM, SIG_DFL);

      if(execvp(p->tasks[0], p->tasks) < 0) {
	perror("Execution error ");
	result = TRUE;
      }
      //free_parse_output(p);
      exit(SUCCESS);
    } else if (pid > 0) {
      if(setpgid(getpid(), getpid()) < 0) {
	perror("Parent: Set parent gid in parent failed: ");
	//free_parse_output(p);
	return TRUE;
      } else if(setpgid(pid, pid) < 0) {
	perror("Parent: set child gid in parent failed: ");
	//free_parse_output(p);
	return TRUE;
      }
      if(tcsetpgrp(mysh_fd, getpgid(pid)) != SUCCESS) {
	perror("Setting child process to foreground failed\n");
	//free_parse_output(p);
	return TRUE;
      }

      tcgetattr(mysh_fd, &jterm);
      waitpid(pid, &stat, WUNTRACED);
      if(WIFSTOPPED(stat)){
	sigprocmask(SIG_BLOCK, &sset, NULL);
	job_node* newjob = new_node(dlist_size(sus_bg_jobs) + 1, suspended, pid,  getpgid(pid), task, NULL, NULL);
	newjob->jmode = jterm;
	dlist_push_end(sus_bg_jobs, newjob);
	sigprocmask(SIG_UNBLOCK, &sset, NULL);
      }
      if(tcsetpgrp(mysh_fd, shell_gpid) == FAILURE) {
	perror("Setting shell back to foreground: ");
	//free_parse_output(p);
	return TRUE;
      }
      if(tcsetattr(mysh_fd, TCSADRAIN, &mysh) == FAILURE) {
	perror("Giving terminal control back to shell: ");
	///free_parse_output(p);
	return TRUE;
      }
    }
    //free_parse_output(p);
  }
  return result;
}

/* ============================ Free Memory ============================= */
void free_joblists() {
  dlist_free(sus_bg_jobs);
}

/*
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
*/

/* =========================== useful function =========================== */
/*
  - convert a string of ints to integer
  - allows non-digit chars inside the string, will extract the number and calculate the integer
  - for instance: "#1#2#2" converts to 122
*/
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

/* ================================ MAIN ============================= */
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
    if (input==NULL){
      run = TRUE;
      continue;
    }
    parse_output* newline = parse_input(input, "\n");
    if(newline->num == 1) {
      run = TRUE;
      //free_parse_output(newline);
      continue;
    }
    parse_output* jobs = parse_input(newline->tasks[0], ";");
    if(jobs->num == 1 && strcmp(newline->tasks[0], ";") == 0) {
      run = TRUE;
      printf("Invalid input\n");
      //free_parse_output(jobs);
      //free_parse_output(newline);
      continue;
    }
    int symbolnum = 0;
    for(int i = 0; i < jobs->num; i++) {
      if(strcmp(jobs->tasks[i], ";") == 0) {
	jobs->tasks[i] = NULL;
	symbolnum ++;
      }
    }

    int jobnum = jobs->num - symbolnum;
    int job = 0;
    for(int i = 0; i < jobs->num; i++) {
      if(jobs->tasks[i] != NULL){
	job = i;
	break;
      }
    }
    for(int i = job; i < jobnum; i ++) {
      parse_output* smalljob = parse_input(jobs->tasks[job], "&");
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
      job += 2;
      ////free_parse_output(smalljob);
    }
    //free_parse_output(jobs);
    //free_parse_output(newline);
    free(input);
  } while (run);
  free_joblists();
}
