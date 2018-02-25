#include <string.h>

char* delimiter = ";&";

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

char* get_next_token(tokenizer* t){
  char* substring = "";
  for (int i=0, char* cur = t->pos; i<strlen(t->str); i++, cur = t->pos+i){
    for (int j=0; j <strlen(delimiter); j++){
      if (*cur == *(delimiter+j)){
        if(strcmp(substring, "")==0){
          return cur;
        }else{
          return substring;
        }
      }
    }
    substring = strcat(substring, cur);
  }
  return NULL;
}

int main(int argc, char** argv){
  char* buffer = NULL;
  size_t num;
  size_t buffer_size;
  num = getline(&buffer, &buffer_size; STDIN_FILENO);
}
