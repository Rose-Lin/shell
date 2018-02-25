
typedef struct tokenizer{
  char *str;  //the string to parse
  char* pos;  //position in string
  char* delim;
}tokenizer;

tokenizer* init_tokenizer(char* str, char* delim);

char* get_next_token(tokenizer* t);
