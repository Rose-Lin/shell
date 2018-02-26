#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "tokenizer.h"

tokenizer* init_tokenizer(char* str, char* delim){
  tokenizer* t = malloc(sizeof(tokenizer));
  t->str = malloc(sizeof(char) * strlen(str));
  strncpy(t->str, str, strlen(str));
  t->pos = str;
  t->delim = malloc(sizeof(char) * strlen(delim));
  strncpy(t->delim, delim, strlen(delim));
  return t;
}

char* get_next_token(tokenizer* t) {
  printf("current pos is %s\n", t->pos);
  if(t->pos == NULL) {return NULL; }
  char* delim = t->delim;
  int slen= strlen(t->pos);
  for(int index = 0; index < slen; index++) {
    for(int d = 0; d < strlen(delim); d ++) {
      //printf("testing delimiter %c and current pos %s\n", delim[d], (t->pos + index));
      if(delim[d] == t->pos[index]) {
        char* result = t->pos + index;
        t->pos += (index + 1);
        return result;
      }
    }
  }
  return NULL;
}

void free_tokenizer(tokenizer* t){
  free(t->str);
  free(t->delim);
  free(t);
}
