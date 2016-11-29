#include <proc.h>
#include <stdio.h>
#include <stdarg.h>
#include <syscall.h>
#include <x86.h>
#include <string.h>

#define BUFLEN 1024
#define MAXARGS 10
static char linebuf[BUFLEN];
int buffer_index;

char *
readline(const char *prompt)
{
  int i;
  char c;

  if (prompt != NULL)
    printf("%s", prompt);

  i = 0;
  while (1) {
    c = getc();
    if (c < 0) {
      printf("read error: %e\n", c);
      return NULL;
    } else if ((c == '\b' || c == '\x7f') && i > 0) {
      printf("\b");
      i--;
    } else if (c >= ' ' && i < BUFLEN-1) {
      printf("%c", c);
      linebuf[i++] = c;
    } else if (c == '\n' || c == '\r') {
      printf("\n");
      linebuf[i] = 0;
      return linebuf;
    }
  }
}

// couldnt find ctype.h...
int isspace(char c) {
  if (c == ' ' || c == '\t' || c == '\v' || c == '\n' || c == '\f' || c == '\r') {
    return 1;
  } else {
    return 0;
  }
}

char * parse_arg(char arg[BUFLEN]) {
  int start;
  int moved;
  while (isspace(linebuf[buffer_index]) && linebuf[buffer_index] != '\0') buffer_index ++;
  moved = start = 0;
  while (!isspace(linebuf[buffer_index]) && linebuf[buffer_index] != '\0' && start < BUFLEN) {
    arg[start] = linebuf[buffer_index];
    start++;
    buffer_index++;
  }
  arg[start] = '\0';
  while (isspace(linebuf[buffer_index]) && linebuf[buffer_index] != '\0') buffer_index ++;
  if (moved == start) {
    return NULL;
  } else {
    return arg;
  }

}

void parse_args(int *argc, char argv[MAXARGS][BUFLEN]) {
  *argc = 0;
  buffer_index = 0;
  while (strlen(linebuf) > buffer_index && *argc < MAXARGS) {
    if (parse_arg(argv[*argc]) != NULL) {
      (*argc)++;
    }
  }   
}

int main (int argc, char **argv)
{
    printf("shell started.\n");
    char arg_array[MAXARGS][BUFLEN];
    int arg_count;

    while(1){
      readline("$ ");
      parse_args(&arg_count, arg_array);
      printf("You said: %s\n", linebuf);

      int i;
      for (i = 0; i < arg_count; i++) {
        printf("Argument %d: %s\n", i, arg_array[i]);
      }
      printf("\n");

      //TODO: switch/case on possible commands for argv[0]
    }
    return 0;
}
