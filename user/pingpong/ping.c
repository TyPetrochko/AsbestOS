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
    char arg_array[MAXARGS][BUFLEN], buff[BUFLEN];
    int arg_count, fd, read;

    // Try to read from current directory
    //  1) Sanity check
    //  2) sys_open initializes the disk log (if we omit this, mkdir fails)
    fd = sys_open(".", O_RDONLY);
    if(fd == -1)
      printf("ls failed to open cwd\n");
    if(sys_close(fd) == -1)
      printf("ls failed to close cwd\n");

    while(1){
      readline("$ ");
      parse_args(&arg_count, arg_array);

      if(!strcmp(arg_array[0], "ls")){
        // LS
        sys_ls(buff, BUFLEN);
        printf("%s\n", buff);
      }else if(!strcmp(arg_array[0], "cd")){
        // CD
        if(arg_count < 2 || sys_chdir(arg_array[1]) == -1)
          printf("cd to directory %s failed\n", arg_array[1]);
      }else if(!strcmp(arg_array[0], "touch")){
        // TOUCH
        if(arg_count < 2)
          printf("usage: touch <filename>\n");
        else if((fd = sys_open(arg_array[1], O_CREATE)) == -1)
          printf("touch could not create file %s\n", arg_array[1]);
        else if(sys_close(fd) == -1)
          printf("touch couldn't close file %s\n");
      }else if(!strcmp(arg_array[0], "mkdir")){
        // MKDIR
        if(arg_count < 2)
          printf("usage: mkdir <dirname>\n");
        else if(sys_mkdir(arg_array[1]) == -1)
          printf("mkdir failed to make directory %s\n", arg_array[1]);
      }else{
        // TODO more here!
        printf("unrecognized command: %s\n", arg_array[0]);
      }
    }
    return 0;
}
