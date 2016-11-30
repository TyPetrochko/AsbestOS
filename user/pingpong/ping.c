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

void ls(char arg_array[MAXARGS][BUFLEN], int arg_count, char *buff){
  sys_ls(buff, BUFLEN);
  printf("%s\n", buff);
}

void cd(char arg_array[MAXARGS][BUFLEN], int arg_count, char *buff, char *cwd){
  int i;
  if(arg_count < 2 || sys_chdir(arg_array[1]) == -1)
    printf("cd to directory %s failed\n", arg_array[1]);
  else{ /* success */
    if(*(arg_array[1]) == '/'){
      strcpy(cwd, arg_array[1]);
    }else if(!strcmp(arg_array[1], ".")){
      /* do nothing */
    }else if(!strcmp(arg_array[1], "..")){
      // turn last slash into a nullchar
      for(i = strlen(cwd); i >= 0; i--){
        if(i == 0){
          cwd[1] = '\0';
        }
        else if(cwd[i] == '/'){
          cwd[i] = '\0';
          break;
        }
      }
    }else{
      if(cwd[strlen(cwd) - 1] != '/')
        strcpy(cwd + strlen(cwd), "/");
      strcpy(cwd + strlen(cwd), arg_array[1]);
    }
  }
}

void touch(char arg_array[MAXARGS][BUFLEN], int arg_count, char *buff){
  int fd;
  if(arg_count < 2)
    printf("usage: touch <filename>\n");
  else if((fd = sys_open(arg_array[1], O_CREATE)) == -1)
    printf("touch could not create file %s\n", arg_array[1]);
  else if(sys_close(fd) == -1)
    printf("touch couldn't close file %s\n");
}

void mk_dir(char arg_array[MAXARGS][BUFLEN], int arg_count, char *buff){
  //printf("%s %s\n", arg_array[0], arg_array[1]);
  if(arg_count < 2)
    printf("usage: mkdir <dirname>\n");
  else if(sys_mkdir(arg_array[1]) == -1)
    printf("mkdir failed to make directory %s\n", arg_array[1]);
}

void cat(char arg_array[MAXARGS][BUFLEN], int arg_count, char *buff){
  int fd, read;
  if(arg_count < 2)
    printf("usage: cat <filename>");

  if(fd = sys_open(arg_array[1], O_RDONLY) == -1)
    printf("cat: could not open file %s\n", arg_array[1]);

  // zero the buffer
  buff[0] = '\0';
  while((read = sys_read(fd, buff, BUFLEN - 1)) == BUFLEN - 1){
    buff[read] = '\0';
    printf("%s", buff);
  }

  // flush remaining bytes
  buff[read] = '\0';
  printf("%s\n", buff);

  if(sys_close(fd) == -1)
    printf("cat: couldn't close file %s\n", arg_array[1]);
}

void rm(char arg_array[MAXARGS][BUFLEN], int arg_count) {
  if (arg_count < 2) {
    printf("usage: rm <filename>\n");
  } else if (sys_unlink(arg_array[1]) == -1){
    printf("error removing file");
  }
}

void cp(char arg_array[MAXARGS][BUFLEN], int arg_count, char *buff) {
  int src, dest, read;
  if(arg_count < 3) {
    printf("usage: cp <src> <dest>");
    return;
  }    

  if((src = sys_open(arg_array[1], O_RDONLY)) == -1)
    printf("cp: could not open file %s\n", arg_array[1]);
  else if((dest = sys_open(arg_array[2], O_CREATE )) == -1)
    printf("cp: could not open file %s\n", arg_array[2]);
  else if(sys_close(dest) == -1)
    printf("cp: couldn't close file %s\n", arg_array[2]);
  else if((dest = sys_open(arg_array[2], O_WRONLY )) == -1)
    printf("cp: could not open file %s\n", arg_array[2]);
  else {

    // zero the buffer
    buff[0] = '\0';
    while((read = sys_read(src, buff, BUFLEN - 1)) == BUFLEN - 1){
      buff[read] = '\0';
      if (sys_write(dest, buff, read) == -1) {
        printf("Write failed\n");
      }
    }

    // flush remaining bytes
    buff[read] = '\0';
    if (read > 0) {
      if (sys_write(dest, buff, read) == -1) {
        printf("Final write failed\n");
      }
    }

    if(sys_close(src) == -1)
      printf("cp: couldn't close file %s\n", arg_array[1]);
   
    if(sys_close(dest) == -1)
      printf("cp: couldn't close file %s\n", arg_array[2]);
  }
}

void mv(char arg_array[MAXARGS][BUFLEN], int arg_count, char *buff) {
  if (arg_count < 3) {
    printf("Usage: mv <src> <dest>");
    return;
  }
  cp(arg_array, arg_count, buff);
  rm(arg_array, arg_count);
}

void echo(char arg_array[MAXARGS][BUFLEN], int arg_count, char *buff){
  int i;
  for(i = 1; i < arg_count; i++){
    printf("%s", arg_array[i]);
    if(i + 1 != arg_count)
      printf(" ");
  }
  printf("\n");
}

void write_string(char * string, char * path) {
  int fd;
  if((fd = sys_open(path, O_CREATE + O_WRONLY)) == -1)
    printf("> could not create file %s\n", path);
  else if (sys_write(fd, string, strlen(string)) == -1)
    printf("> failed to write\n");
  else if(sys_close(fd) == -1)
    printf("> couldn't close file %s\n");

}

int main (int argc, char **argv)
{
    printf("shell started.\n");
    char arg_array[MAXARGS][BUFLEN], buff[BUFLEN], cwd[BUFLEN];
    int arg_count, fd, read;

    // Try to read from current directory
    //  1) Sanity check
    //  2) sys_open initializes the disk log (if we omit this, mkdir fails)
    fd = sys_open(".", O_RDONLY);
    if(fd == -1)
      printf("ls failed to open cwd\n");
    if(sys_close(fd) == -1)
      printf("ls failed to close cwd\n");

    // initialize cwd
    cwd[0] = '/';
    cwd[1] = '\0';

    while(1){
      readline("$ ");
      parse_args(&arg_count, arg_array);

      if (arg_count > 1 && !strcmp(arg_array[1], ">")) {
        //write to file
        if (arg_count > 2) {
          write_string(arg_array[0], arg_array[2]);
        } else {
          //usage?
        }
      } else if(!strcmp(arg_array[0], "ls")){
        // LS
        ls(arg_array, arg_count, buff);
      }else if(!strcmp(arg_array[0], "cp")){
        // CP
        cp(arg_array, arg_count, buff);
      }else if(!strcmp(arg_array[0], "rm")){
        // RM
        rm(arg_array, arg_count);
      }else if(!strcmp(arg_array[0], "mv")){
        // MV
        mv(arg_array, arg_count, buff);
      }else if(!strcmp(arg_array[0], "cd")){
        // CD
        cd(arg_array, arg_count, buff, cwd);
      }else if(!strcmp(arg_array[0], "touch")){
        // TOUCH
        touch(arg_array, arg_count, buff);
      }else if(!strcmp(arg_array[0], "mkdir")){
        // MKDIR
        mk_dir(arg_array, arg_count, buff);
      }else if(!strcmp(arg_array[0], "pwd")){
        // PWD
        printf("%s\n", cwd);
      }else if(!strcmp(arg_array[0], "cat")){
        // MKDIR
        cat(arg_array, arg_count, buff);
      }else if(!strcmp(arg_array[0], "echo")){
        // MKDIR
        echo(arg_array, arg_count, buff);
      }else{
        // TODO more here!
        printf("unrecognized command: %s\n", arg_array[0]);
      }
    }
    return 0;
}
