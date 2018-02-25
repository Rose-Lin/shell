#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "tokenizer.h"

char* delimiter = " ;&";

tokenizer* init_tokenizer(char* str, char delim){
  tokenizer* t = malloc(sizeof(tokenizer));
  t->str = str;
  t->pos = str;
  t->delim = delim;
  return t;
}

char* get_next_token(tokenizer* t) {
  char* token = strchr(t->pos, t->delim);
  if(token != NULL) {
    t->pos = token+1;  // points to the char after the current delim;
  }
  return token;
}

/*
char* get_next_token(tokenizer* t){
  char* substring = "";
  for (int i=0, char* cur = t->pos; i<strlen(t->str); i++, cur = t->pos+i){
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
*/
