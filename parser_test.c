#include <stdio.h>
#include <string.h>
#include <stdlib.h>

# define BUFFSIZE 1024

typedef struct tokenizer{
  char *str;  //the string to parse
  char* pos;  //position in string
  char* delim;
}tokenizer;

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
  if(t->pos == NULL) {return NULL; }
  char* delim = t->delim;
  int slen= strlen(t->pos);
  for(int index = 0; index < slen; index++) {
    for(int d = 0; d < strlen(delim); d ++) {
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

typedef struct parse_output{
  int num;
  char** tasks;
}parse_output;

parse_output* parse_input(char* input, char* delim){
  // char** jobs = malloc(sizeof(char*)*BUFFSIZE);
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
    // printf("cur:%s|\n", cur);
    char* next_cur = get_next_token(t);
    // printf("nex:%s|\n", next_cur);
    if (next_cur){
      new_length = strlen(next_cur);
    }else{
      // printf("next_cur null\n" );
      new_length = 0;
    }
    /*the length of the token*/
    int copy_length = original_length-new_length;
    // printf("%d\n", copy_length);
    // if (next_cur == NULL){
    //   printf("%s\n", "+++++++++");
    //   jobs[count_token] = malloc(sizeof(char*)*(copy_length+1));
    //   strncpy(jobs[count_token], cur, copy_length);
    //   /*add a 0 to the end*/
    //   *(jobs[count_token]+copy_length+1) =0;
    //   /*check if the jobs is running out of size*/
    //   if (occupy >= BUFFSIZE){
    //     jobs = (char**)realloc(jobs, BUFFSIZE+count_token);
    //     occupy -= BUFFSIZE;
    //   }
    //   printf("%s|\n", jobs[count_token]);
    //   count_token ++;
    //   occupy ++;
    // }else{
    if (!next_cur || strcmp(cur, next_cur)!=0){
        // printf("%s\n", "+++++++++====");
        parse_result->tasks[count_token] = malloc(sizeof(char*)*(copy_length+1));
        strncpy(parse_result->tasks[count_token], cur, copy_length);
        /*add a 0 to the end*/
        *(parse_result->tasks[count_token]+copy_length+1) =0;
        /*check if the jobs is running out of size*/
        if (occupy >= BUFFSIZE){
          printf("occupy:%d\n", BUFFSIZE+count_token);
          parse_result->tasks= (char**)realloc(parse_result->tasks, BUFFSIZE+count_token);
          occupy -= BUFFSIZE;
          printf("changed occupy:%d\n", occupy);
        }
        printf("%s|\n", parse_result->tasks[count_token]);
        count_token ++;
        occupy ++;
    }
    if (next_cur){
      printf("%s\n", "yoyoyo");
      for(int i=0; i<strlen(delim); i++){
        if(*next_cur == *(delim+i)){
          // printf("%c\n", *(delim+i));
          char new_token[2];
          new_token[0] = delim[i];
          new_token[1] = '\0';
          parse_result->tasks[count_token] = malloc(sizeof(char*)*2);
          // printf("new_token:%s|\n",new_token );
          next_cur ++;
          /*check if the jobs is running out of size*/
          if (occupy >= BUFFSIZE){
            printf("size:%d\n", BUFFSIZE+count_token);
            parse_result->tasks = (char**)realloc(parse_result->tasks, BUFFSIZE+count_token);
            occupy -= BUFFSIZE;
            printf("changed occupy:%d\n", occupy);
          }
          /*string copy*/
          strncpy(parse_result->tasks[count_token], new_token,strlen(new_token));
          // printf("special_token:%s| + count_token: %d\n", jobs[count_token], count_token);
          count_token ++;
          occupy ++;
        }
      }
    }
    cur = next_cur;
    // printf("%s\n", "~~~~~");
    if (next_cur && *next_cur =='\0'){
      next_cur = NULL;
      cur = NULL;
    }
  }
  parse_result->tasks = parse_result->tasks;
  parse_result->num = count_token;
  return parse_result;
}

int main(){
  char* input = "emacs ; wehsoe ; so& sej;sieweo% slne&soej;seiose&slifee&slfieoi&sfkue\n";
  // char* input = ";";
  // char* input = "\n";
  // char* input = " ";
  // char* input = ";;;;;;;;; ;  %&%&^5^829hd&% \n fd &%\nsd&&&&&;*";
  // char* input = ";;;;;;;;; ;";
  // char* input = "wke;sfeu soei^lf&oeif\n";
  parse_output* po;
  po = parse_input(input, ";&%\n");
  printf("%s\n", "~~~~~~~~~~~~~~~~~~");
  char** start = po->tasks;
  printf("%d\n", po->num);
  // printf("%s\n", *(po->tasks+4));
  for (int i=0; i<po->num; i++){
    printf("%d\n", i);
    printf("%s|\n", *(po->tasks+i));
  }
}
