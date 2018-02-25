
void* create_shared_memory(size_t size);

int parse(char* input, size_t num);

int check_special_symbols(char* input);

int execute_input(char** tokens, job_node* job);
