
void init_sems();

void signal_hanlders();

void print_jobs();

void* create_shared_memory(size_t size);

char* read_input(); // simply read in input

int check_special_symbols(char* input); // check for special characters

int parse_input(char* input); // allocate an global array for storing tokens

int execute_input(char** tokens, job_node* job);

void update_list(pid_t gid, int flag);
