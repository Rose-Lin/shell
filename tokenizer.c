#include <string.h>
#include <stdio.h>
#include <stdlib.h>

char* delimiter = ";&";

typedef struct tokenizer{
  char *str;  //the string to parse
  char *pos;  //position in string
}tokenizer;

tokenizer* create_new_tokenizer(char* str){
  tokenizer* t = malloc(sizeof(tokenizer));
  t->str = str;
  t->pos = str;
  return t;
}

char* get_next_token(tokenizer* t, char substring[600]){
  char* cur = t->pos;
  for (int i=0; i<strlen(t->str); i++, cur = t->pos+i){
    for (int j=0; j <strlen(delimiter); j++){
      char ch = cur[0];
      char d = delimiter[j];
      if (ch == d){
        if(strcmp(substring, "")==0){
          return cur;
        }else{
          return substring;
        }
      }
    }
    strcat(substring, cur);
  }
  return NULL;
}

int main(int argc, char** argv){
  char* buffer = "This is my shell c;ode.\n";
  size_t num = 10;
  size_t buffer_size;
  // num = getline(&buffer, &buffer_size, 0);
  // printf("%d\n", num);
  if (num){
    tokenizer* t = create_new_tokenizer(buffer);
    char substring[600];
    char* s = get_next_token(t, substring);
    printf("%s\n",s);
    // free(t);
  }
  return 0;
}
