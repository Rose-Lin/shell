#include <stdlib.h>


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
	struct tokenizer* t = init_tokenizer(input, )
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
  char* delimiter = ";&";
  tokenizer* t = init_tokenizer("se xdfnsfeos;skei&siefn;se", delimiter);
  printf("%s\n", get_next_token(t));
}

// int parse(char* buffer, size_t byte_num){
//   int i = 0;
//   if(! byte_num){
//     return 0;
//   }
//   if(strcmp(buffer, "") == 0){
//     return 0;
//   }
// }

void* create_shared_memory(size_t size){
  int protection = PROT_READ | PROT_WRITE;
  int visibility = MAP_ANONYMOUS | MAP_SHARED;
  return mmap(NULL, size, protection, visibility, 0, 0);
}
