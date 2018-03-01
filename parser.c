#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "parser.h"
#include "tokenizer.h"

# define BUFFSIZE 2048

int search_for_space(char* string){
  /*return the index of the occurrance of spce*/
  /*if space does not occur, return -1*/
  char* p = string;
  for(int i=0; i<strlen(p); i++){
    if(*(p+i) == ' '){
      return i;
    }
  }
  return -1;
}

parse_output* parse_input(char* input, char* delim){
  if (!input){
    return NULL;
  }
  int delete_space = 0;
  if(search_for_space(delim)>=0){
    delete_space = 1;
  }
  parse_output* parse_result = malloc(sizeof(parse_output*));
  parse_result->num = 0;
  parse_result->tasks = malloc(sizeof(char*)*BUFFSIZE);
  tokenizer* t = init_tokenizer(input, delim);
  char * cur = input;
  int original_length = 0;
  int new_length = 0;
  int count_token = 0;
  int occupy = count_token;
  while(cur){
    original_length = strlen(cur);
    char* next_cur = get_next_token(t);
    if (next_cur){
      new_length = strlen(next_cur);
    }else{
      new_length = 0;
    }
    /*the length of the token*/
    int copy_length = original_length-new_length;
    if (!next_cur || strcmp(cur, next_cur)!=0){
      parse_result->tasks[count_token] = malloc(sizeof(char*)*(copy_length+1));
      strncpy(parse_result->tasks[count_token], cur, copy_length);
      /*add a 0 to the end*/
      *(parse_result->tasks[count_token]+copy_length+1) =0;
      /*check if the tasks is running out of size*/
      if (occupy >= BUFFSIZE){
        parse_result->tasks= (char**)realloc(parse_result->tasks, BUFFSIZE+count_token);
        occupy -= BUFFSIZE;
      }
      if (delete_space){
        int index = search_for_space(parse_result->tasks[count_token]);
        *( parse_result->tasks[count_token]+index) = 0;
      }
      count_token ++;
      occupy ++;
    }
    if (next_cur){
      for(int i=0; i<strlen(delim); i++){
        if(*next_cur == *(delim+i)){
          char new_token[2];
          new_token[0] = delim[i];
          new_token[1] = '\0';
          next_cur ++;
          t->pos ++;
          cur = next_cur;
          /*check if the jobs is running out of size*/
          if (occupy >= BUFFSIZE){
            parse_result->tasks = (char**)realloc(parse_result->tasks, BUFFSIZE+count_token);
            occupy -= BUFFSIZE;
          }
          /*string copy*/
          if(!(delete_space && new_token[0]== ' ')){
            parse_result->tasks[count_token] = malloc(sizeof(char*)*2);
            strncpy(parse_result->tasks[count_token], new_token,strlen(new_token));
            count_token ++;
            occupy ++;
          }
        }
      }
    }
    cur = next_cur;
    if (next_cur && *next_cur =='\0'){
      next_cur = NULL;
      cur = NULL;
    }
  }
  parse_result->tasks = parse_result->tasks;
  parse_result->num = count_token;
  free_tokenizer(t);
  return parse_result;
}

void free_parse_output(parse_output* p){
  char** ptask = p->tasks;
  for(int i=0; i<p->num; i++){
    free(ptask[i]);
  }
  free(p);
}
