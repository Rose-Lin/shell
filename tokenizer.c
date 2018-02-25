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
*/
