#include "job_node.h"
#include "dlist.h"

typedef struct parse_output{
  int num;
  char** tasks;
}parse_output;

parse_output* parse_input(char* input, char* delim);

void free_parse_output(parse_output*);
