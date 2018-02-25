
void init_sems();  // checked

void close_sems(); // checked

void init_joblists(); // checked

void free_joblists(); // checked

void sigchild_handler();

void print_jobs(dlist*);

void test_job_list(); //testing joblist

void* create_shared_memory(size_t size);

char* read_input(); // simply read in input   // checked

int check_special_symbols(char* input); // check for special characters  //checked

int parse_input(char* input); // allocate an global array for storing tokens

int execute_input(char** tokens, job_node* job);

void update_list(pid_t gid, int flag);
