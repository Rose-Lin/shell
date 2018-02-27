typedef struct Parse_output{
	int total;
	char** tasks;
}Parse_output;


void init_sems();  // checked

void close_sems(); // checked

void init_joblists(); // checked

void free_joblists(); // checked

void print_jobs(dlist job);  // checked

// signals
int set_up_signals(); // sets up the signals for the shell

void* sigchld_handler(int, siginfo_t*, void*); // signal handler for sigchld

int update_list(pid_t gid, int flag);

void test_job_list(); //testing joblist

void* create_shared_memory(size_t size);

char* read_input(); // simply read in input   // checked

int check_special_symbols(char* input); // check for special characters  //checked

Parse_output parse_input(char* input, char* delim, char** store); // allocate an global array for storing tokens //checked

int execute(char*);

int execute_input(char* task);

int exeute_bg(char** tasks); // needs to parse %
