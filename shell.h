
void init_sems();  // check

void close_sems(); // check

void signal_hanlders();

void print_jobs();

void* create_shared_memory(size_t size);

char* read_input(); // simply read in input   // check

int check_special_symbols(char* input); // check for special characters

int parse_input(char* input); // allocate an global array for storing tokens

int execute_input(char** tokens, job_node* job);

void update_list(pid_t gid, int flag);
