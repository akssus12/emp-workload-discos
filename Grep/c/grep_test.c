#include <stdio.h>
#include <string.h>
#include <stdlib.h>
const char* sentences = "Hello, this is some sample text\n"
                        "This is the second sentence\n"
                        "\n"
                        "The sentence above is just a new line\n"
                        "This is the last sentence.\n";

void parse(const char* input){
  char *start, *end;
  char *string;
  char target[256];
  unsigned count = 0;
  int num_lines = 0;

  // the cast to (char*) is because i'm going to change the pointer, not because i'm going to change the value.
  start = end = (char*) input;

  strcpy(target, "sentence");

  while( (end = strchr(start, '\n')) ){
      int size = end - start;
      printf("size : %d\n", size);

      string = (char *)malloc(sizeof(char) * size + 1);
      strncpy(string, start, size);
      memset(string, 0, size+1);
      printf("string : %s\n", string);
      num_lines++;
      char * token = strtok(string, " ");
      while(token != NULL){
        if(strcmp(token, target) == 0){
            printf("lines : %d\n", num_lines);
        } else {
            printf("not this lines %d\n", num_lines);
        }
        token = strtok(NULL, " ");
      }
      free(string);
      printf("%d %.*s", count++, (int)(end - start + 1), start);
      start = end + 1;
  }
}

int main(void){
  parse(sentences);
}