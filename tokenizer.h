typedef struct tokenizer* tokenizer;

tokenizer* create_new_tokenizer(char* str, char* pos);

char* get_next_token(tokenizer* t);
