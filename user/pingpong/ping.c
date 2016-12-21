#include <proc.h>
#include <stdio.h>
#include <stdarg.h>
#include <syscall.h>
#include <x86.h>
#include <string.h>

#define BUFLEN 1024
#define MAXARGS 10
#define PAGESIZE 4096

/* these aren't included for some reason */
#define T_DIR  1   // Directory
#define T_FILE 2   // File
#define T_DEV  3   // Device

static char linebuf[BUFLEN];
int buffer_index;

int get_filetype(char *path);
void cp_r(char *src, char *dst, char *buff);

int MAP_SIZE = 80*480;
char vga_mem[80*480] __attribute__ ((aligned(PAGESIZE)));

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
  if(arg_count == 1){
    sys_ls(buff, ".", BUFLEN);
    printf("%s\n", buff);
  }else{
    if(get_filetype(arg_array[1]) != T_DIR){
      printf("ls: %s is not a directory\n", arg_array[1]);
      return;
    }
    sys_ls(buff, arg_array[1], BUFLEN);
    printf("%s\n", buff);
  }
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
  if(arg_count < 2)
    printf("usage: mkdir <dirname>\n");
  else if(sys_mkdir(arg_array[1]) == -1)
    printf("mkdir failed to make directory %s\n", arg_array[1]);
}

void cat(char arg_array[MAXARGS][BUFLEN], int arg_count, char *buff){
  int fd, read;
  if(arg_count < 2){
    printf("usage: cat <filename>\n");
    return;
  }

  if(fd = sys_open(arg_array[1], O_RDONLY) == -1){
    printf("cat: could not open file %s\n", arg_array[1]);
    return;
  }

  if(get_filetype(arg_array[1]) != T_FILE){
    printf("cat: %s is not a regular file\n", arg_array[1]);
    return;
  }

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

void rm_dir(char *src, char *buff){
  char *ptr;
  char src_buff[BUFLEN]; // full-path holders
  int a, fd;

  // list all files in src dir
  sys_ls(buff, src, BUFLEN);

  ptr = buff;
  while(*ptr != '\0' && ptr < buff + strlen(buff) + 1){
    // gobble whitespace
    while(*ptr == ' ') ptr++;
    // escape if necessary
    if(*ptr == '\0')
      break;

    // prepare src_buff to build full paths
    strcpy(src_buff, src);
    if(src_buff[strlen(src_buff) - 1] != '/')
      strcpy(src_buff + strlen(src_buff), "/");
    
    // copy in a single word from "ls" into buffer
    for(a = strlen(src_buff);
        *ptr != ' ' && *ptr != '\0';
        src_buff[a++] = *(ptr++)){;
    }

    // null terminate
    src_buff[a] = '\0'; 

    if(get_filetype(src_buff) == T_DIR){
      rm_dir(src_buff, buff);
    }
    if (sys_unlink(src_buff) == -1) {
      printf("rm: error removing %s", src_buff);
    }
  }
}

void rm(char arg_array[MAXARGS][BUFLEN], int arg_count, char * buff) {
  if (arg_count < 2) {
    printf("usage: rm <filename>\n");
  } else if (arg_count > 2 && !strcmp(arg_array[1], "-r")) {
    //recursive
    rm_dir(arg_array[2], buff);
    if (sys_unlink(arg_array[2]) == -1){
      printf("rm: error unlinking dir in rm -r %s\n", arg_array[1]);
    }
  } else if (get_filetype(arg_array[1]) != T_FILE){
    printf("rm: %s is not a file, try \"rm -r\"\n", arg_array[1]);
  } else if (sys_unlink(arg_array[1]) == -1){
    printf("rm: error unlinking file %s\n", arg_array[1]);
  }
}

void copy_file(char *to, char *from, char *buff){
  int src, dest, read;
  if((src = sys_open(from, O_RDONLY)) == -1)
    printf("cp: could not open file %s\n", from);
  else if((dest = sys_open(to, O_CREATE )) == -1)
    printf("cp: could not open file %s\n", to);
  else if(sys_close(dest) == -1)
    printf("cp: couldn't close file %s\n", to);
  else if((dest = sys_open(to, O_WRONLY )) == -1)
    printf("cp: could not open file %s\n", to);
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
      printf("cp: couldn't close file %s\n", from);
   
    if(sys_close(dest) == -1)
      printf("cp: couldn't close file %s\n", to);
  }
}

void cp_dir(char *src, char *dst, char *buff){
  char *ptr;
  char dest_buff[BUFLEN], src_buff[BUFLEN], extra_buff[BUFLEN]; // full-path holders
  int a, b, fd;

  // list all files in src dir
  sys_ls(buff, src, BUFLEN);

  ptr = buff;
  while(*ptr != '\0' && ptr < buff + strlen(buff) + 1){
    // gobble whitespace
    while(*ptr == ' ') ptr++;
    // escape if necessary
    if(*ptr == '\0')
      break;

    // prepare dest_buff and src_buff to build full paths
    strcpy(src_buff, src);
    if(src_buff[strlen(src_buff) - 1] != '/')
      strcpy(src_buff + strlen(src_buff), "/");
    
    strcpy(dest_buff, dst);
    if(dest_buff[strlen(dest_buff) - 1] != '/')
      strcpy(dest_buff + strlen(dest_buff), "/");

    // copy in a single word from "ls" into both buffers
    for(a = strlen(src_buff), b = strlen(dest_buff);
        *ptr != ' ' && *ptr != '\0';
        src_buff[a++] = *ptr, dest_buff[b++] = *(ptr++)){
    }

    // null terminate
    src_buff[a] = '\0';
    dest_buff[b] = '\0';
    

    if(get_filetype(src_buff) == T_DIR){
      if(mkdir(dest_buff) == -1){
        printf("cp: mkdir on %s failed \n",dest_buff);
        return;
      }

      cp_dir(src_buff, dest_buff, extra_buff);
    }else{
      copy_file(dest_buff, src_buff, extra_buff);
    }
  }
}

void cp_r(char *src, char *dst, char *buff){
  // make the file first, then do recursive copy
  char new_dir[BUFLEN];
  char *beginning;

  // not sure why we need this
  sys_ls(buff, src, BUFLEN);

  // find the name of src
  beginning = src + strlen(src);
  while(beginning != src){
    if(*beginning != '/')
      beginning --;
    else{
      beginning ++;
      break;
    }
  }

  strcpy(new_dir, dst);
  if(new_dir[strlen(new_dir) - 1] != '/')
    strcpy(new_dir + strlen(new_dir), "/");
  strcpy(new_dir + strlen(new_dir), beginning);

  
  if(sys_mkdir(new_dir) == -1){
    printf("mkdir failed to make directory %s\n", new_dir);
    return;
  }

  cp_dir(src, new_dir, buff);
}

void cp(char arg_array[MAXARGS][BUFLEN], int arg_count, char *buff) {
  int src, dest, read;
  if(arg_count < 3) {
    printf("usage: cp [-r] <src> <dest>\n");
    return;
  }

  if(!strcmp(arg_array[1], "-r") && arg_count >= 4)
    cp_r(arg_array[2], arg_array[3], buff);
  else
    copy_file(arg_array[2], arg_array[1], buff);
}

void mv(char arg_array[MAXARGS][BUFLEN], int arg_count, char *buff) {
  if (arg_count < 3) {
    printf("Usage: mv <src> <dest>\n");
    return;
  }
  copy_file(arg_array[2], arg_array[1], buff);
  rm(arg_array, arg_count, buff);
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

void append_string(char *string, char *path, char* buff) {
  int fd, read;
  if((fd = sys_open(path, O_RDWR)) == -1)
    printf("> could not open file %s\n", path);
  else {
    while((read = sys_read(fd, buff, BUFLEN - 1)) == BUFLEN - 1);
    if (sys_write(fd, string, strlen(string)) == -1)
      printf("> failed to write\n");
    else if(sys_close(fd) == -1)
      printf("> couldn't close file %s\n");
    }
}

int get_filetype(char *path){
  int fd, ret;
  struct file_stat st;

  if((fd = sys_open(path, O_RDONLY)) == -1){
    printf("get_filetype: couldn't open file %s\n", path);
    return -1;
  }

  if(sys_fstat(fd, &st) == -1){
    printf("get_filetype: couldn't fstat file %s\n", path);
    return -1;
  }

  if(st.type != T_DIR && st.type != T_FILE && st.type != T_DEV){
    printf("get_filetype: file %s has corrupted type: %d\n", path, st.type);
    return -1;
  }

  ret = st.type;

  if(sys_close(fd) == -1){
    printf("get_filetype: couldn't close file %s\n", path);
    return -1;
  }

  return ret;
}

int main (int argc, char **argv)
{
    printf("shell started.\n");
    char arg_array[MAXARGS][BUFLEN], buff[BUFLEN], cwd[BUFLEN];
    int arg_count, fd, read;



    //setup vga mem TODO: put this somewhere else
    printf("trying to set up vga map\n");
    //n doesnt matter atm
    sys_vga_map(&vga_mem);
    //try coloring
    // for(int i = 0; i < MAP_SIZE; i++){
    //   vga_mem[i] = 0x00;
    // }
    //sys_switch_mode(0);
    // for(int i = 0; i < MAP_SIZE / 16; i++){
    //   vga_mem[i] = 0xAA;
    // }
    // for(int i = 0; i < MAP_SIZE / 16; i++){
    //   vga_mem[i] = 0xAA;
    // }


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
      // zero line buffer
      linebuf[0] = '\0';
      readline("$ ");

      parse_args(&arg_count, arg_array);

      if (arg_count > 1 && !strcmp(arg_array[1], ">")) {
        //write to file
        if (arg_count > 2) {
          write_string(arg_array[0], arg_array[2]);
        } else {
          //usage?
        }
      } else if (arg_count > 1 && !strcmp(arg_array[1], ">>")) {
        //write to file
        if (arg_count > 2) {
          append_string(arg_array[0], arg_array[2], buff);
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
        rm(arg_array, arg_count, buff);
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
      } else if (!strcmp(arg_array[0], "video")){
        sys_switch_mode(1);
        // for(int i = 0; i < MAP_SIZE; i++){
        //   vga_mem[i] = 0x11;
        // }
      } else if (!strcmp(arg_array[0], "novideo")){
        sys_switch_mode(0);
      }else if (!strcmp(arg_array[0], "draw")) {
        for(int i = 0; i < MAP_SIZE/16; i++){
          vga_mem[i] = 0x11;
        }
      }else {
        // TODO more here!
        printf("unrecognized command: %s\n", arg_array[0]);
         
      }
      linebuf[0] = '\0';
    }
    return 0;
}
