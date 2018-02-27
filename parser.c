#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "tokenizer.h"
#include "parser.h"

// read in the input and add one jobnode(with original input)
char* read_input() {
  size_t readn;
  char* input = NULL;
  printf("mysh > ");
  if(getline(&input, &readn, stdin) < 0) {
    printf("getline from stdin failed! \n");
    free(input);
    input = NULL;
  }

  printf("4\n");
  job_node* jn = new_node(dlist_size(all_joblist)+1, NOTKNOWN, NOTKNOWN, NOTKNOWN, input, NULL, NULL);
  printf("5\n");
  jn->original_input = malloc(sizeof(char) * (strlen(input) + 1));
  sprintf(jn->original_input, input);
  jn->original_input[strlen(input)] = '\0';
  dlist_push_end(all_joblist, jn);
  return input;
}


parse_output* parse_input(char* input, char* delim) {
 char* cur = input;
 int total = 0;
 int size = BUFSIZE;
 parse_output* po = malloc(sizeof(struct parse_output));
 if(po == NULL) {
   printf("malloc failed\n");
   return NULL;
 }
 struct tokenizer* t = init_tokenizer(input, delim);
 char* token = get_next_token(t);
 po->tasks = (char**)malloc(sizeof(char*) * size);
 if(token == NULL) {
   int len = strlen(input);
   if((po->tasks[total] = malloc(sizeof(char) *  len)) == NULL) {
     printf("malloc failed\n");
     return NULL;
   }
   sprintf(po->tasks[total], input);
   po->tasks[total][len] = '\0';
   po->num = 1;
   return po;
 } else if (strcmp(token, "\n") == 0) {
   int len = strlen(input) - 1;
   if(len == 0) {
     free(po->tasks);
     free(po);
     return NULL;
   }
   if((po->tasks[total] = malloc(sizeof(char) *  strlen(input))) == NULL) {
     printf("malloc failed\n");
     return NULL;
   }
   sprintf(po->tasks[total], input);
   po->tasks[total][len] = '\0';
   po->num = 1;
   return po;
 }

 while(token != NULL && strcmp(token, "\n") != 0) {
   int strlength = strlen(cur) - strlen(token);
   int malloclength = strlength + 1;
   if(*token == '&') { //if the current one is background job
     malloclength ++;
   }
   printf("malloc length is %d\n", malloclength);
   po->tasks[total] = malloc(sizeof(char) *  malloclength);
   sprintf(po->tasks[total], "%-*s", malloclength - 1, cur);
   po->tasks[total][malloclength - 1] = '\0';
   total += 1;
   if(total >= BUFSIZE) {
     size += BUFSIZE;
     po->tasks = (char**)realloc(po->tasks, size);
   }
   cur = token;
   token = get_next_token(t);
 }
 po->num = total;
 return po;
}
