typedef struct parse_output{
  int num;
  char** tasks;
}parse_output;

char* read_input();
parse_output* parse_input(char* input, char* delim);
