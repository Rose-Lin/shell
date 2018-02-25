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

#include "dlist.h"
#include "job_node.h"
#include "shell.h"
#include "tokenizer.h"

#define FALSE 0
#define TRUE 1
#define BUFF_SIZE 5

// enums
enum status{background, foreground, suspended};
enum flags{fg_to_bg,
	   sus_to_bg,
	   bg_to_fg,
	   fg_to_kill,
	   exit_shell,
	   start_bg};
enum special_inputs{delim_bg = '&', delim_mult = ';'};
// semaphores
sem_t* all;
sem_t* bglist;
sem_t* suslist;

// job lists
dlist* all_joblist;
dlist* background_joblist;
dlist* suspended_joblist;

// globals
char special_delim[] = "&;";
char all_delim[] = " &;";
int multi_jobs = FALSE;
int launch_bg = FALSE;
pid_t cur_fg_job; // keeps track of the pid of the current foreground job
int execjob_num = 0; // number of jobs to be executed during one input

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

// read in the input
char* read_input() {
  size_t readn;
  char* input = NULL;
  printf("mysh > ");
  if(getline(&input, &readn, stdin) < 0) {
    printf("getline from stdin failed! \n");
    free(input);
    input = NULL;
  }
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
	return execjob_num;
}

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
  printf("%s\n",dl->tail->original_input );
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

int main(int argc, char* argv[]){
  int i = 0;
  void* shmem_all_joblist = create_shared_memory(sizeof(dlist*));
  void* shmem_background_joblist = create_shared_memory(sizeof(dlist*));
  void* shmem_suspended_joblist = create_shared_memory(sizeof(dlist*));
  memcpy(shmem_all_joblist, (void*)&all_joblist, sizeof(dlist*));
  memcpy(shmem_suspended_joblist,(void*)&suspended_joblist,sizeof(dlist*));
  memcpy(shmem_background_joblist,(void*)&background_joblist,sizeof(dlist*));
  size_t byte_num = 0;
  size_t buff_size = BUFF_SIZE;
  char* buffer = NULL;
  // byte_num = getline(&buffer, &buff_size, STDIN_FILENO);
  char* delimiter = ";& ";
  tokenizer* t = init_tokenizer("se xdfnsfeos;skei&siefn;se", delimiter);
  printf("%s\n", get_next_token(t));
  test_job_list();
}

void* create_shared_memory(size_t size){
  int protection = PROT_READ | PROT_WRITE;
  int visibility = MAP_ANONYMOUS | MAP_SHARED;
  return mmap(NULL, size, protection, visibility, 0, 0);
}
