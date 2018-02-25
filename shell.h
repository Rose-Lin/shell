
void* create_shared_memory(size_t size);

int parse(char* input);

int check_special_symbols(char* input);

int execute_input(char** tokens, job_node* job);
