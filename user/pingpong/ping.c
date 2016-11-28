#include <proc.h>
#include <stdio.h>
#include <stdarg.h>
#include <syscall.h>
#include <x86.h>

#define BUFLEN 1024
static char linebuf[BUFLEN];

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

int main (int argc, char **argv)
{
    printf("shell started.\n");

    while(1){
      readline("$ ");
      printf("You said: %s\n", linebuf);
    }
    return 0;
}
