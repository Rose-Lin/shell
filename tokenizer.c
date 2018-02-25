typedef struct tokenizer{
  char *str;  //the string to parse
  char* pos;  //position in string
}tokenizer;

tokenizer* create_new_tokenizer(char* str){
  tokenizer* t = malloc(sizeof(tokenizer));
  t->str = str;
  t->pos = str;
  return t;
}
