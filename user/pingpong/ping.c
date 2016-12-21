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

// Special keycodes
#define KEY_HOME  0xE0
#define KEY_END   0xE1
#define KEY_UP    0xE2
#define KEY_DN    0xE3
#define KEY_LF    0xE4
#define KEY_RT    0xE5
#define KEY_PGUP  0xE6
#define KEY_PGDN  0xE7
#define KEY_INS   0xE8
#define KEY_DEL   0xE9

#define SHIFT   (1<<0)
#define CTL   (1<<1)
#define ALT   (1<<2)

#define DELAY 50 //iterations of syscall
#define BLINK_DELAY 8000 //blink 

static char linebuf[BUFLEN];
int buffer_index;

int get_filetype(char *path);
void cp_r(char *src, char *dst, char *buff);


/* VGA interface */
#define MAP_SIZE (80*480)
char vga_mem[MAP_SIZE] __attribute__ ((aligned(PAGESIZE)));

void vga_init() {
  sys_vga_map(&vga_mem);
}

void vga_set_frame(int frame) {
  sys_set_frame(frame);
}

char font8x8_basic[128][8] = {
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},   // U+0000 (nul)
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},   // U+0001
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},   // U+0002
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},   // U+0003
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},   // U+0004
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},   // U+0005
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},   // U+0006
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},   // U+0007
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},   // U+0008
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},   // U+0009
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},   // U+000A
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},   // U+000B
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},   // U+000C
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},   // U+000D
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},   // U+000E
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},   // U+000F
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},   // U+0010
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},   // U+0011
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},   // U+0012
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},   // U+0013
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},   // U+0014
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},   // U+0015
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},   // U+0016
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},   // U+0017
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},   // U+0018
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},   // U+0019
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},   // U+001A
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},   // U+001B
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},   // U+001C
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},   // U+001D
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},   // U+001E
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},   // U+001F
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},   // U+0020 (space)
    { 0x18, 0x3C, 0x3C, 0x18, 0x18, 0x00, 0x18, 0x00},   // U+0021 (!)
    { 0x6C, 0x6C, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},   // U+0022 (")
    { 0x6C, 0x6C, 0xFE, 0x6C, 0xFE, 0x6C, 0x6C, 0x00},   // U+0023 (#)
    { 0x30, 0x7C, 0xC0, 0x78, 0x0C, 0xF8, 0x30, 0x00},   // U+0024 ($)
    { 0x00, 0xC6, 0xCC, 0x18, 0x30, 0x66, 0xC6, 0x00},   // U+0025 (%)
    { 0x38, 0x6C, 0x38, 0x76, 0xDC, 0xCC, 0x76, 0x00},   // U+0026 (&)
    { 0x60, 0x60, 0xC0, 0x00, 0x00, 0x00, 0x00, 0x00},   // U+0027 (')
    { 0x18, 0x30, 0x60, 0x60, 0x60, 0x30, 0x18, 0x00},   // U+0028 (()
    { 0x60, 0x30, 0x18, 0x18, 0x18, 0x30, 0x60, 0x00},   // U+0029 ())
    { 0x00, 0x66, 0x3C, 0xFF, 0x3C, 0x66, 0x00, 0x00},   // U+002A (*)
    { 0x00, 0x30, 0x30, 0xFC, 0x30, 0x30, 0x00, 0x00},   // U+002B (+)
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x30, 0x30, 0x60},   // U+002C (,)
    { 0x00, 0x00, 0x00, 0xFC, 0x00, 0x00, 0x00, 0x00},   // U+002D (-)
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x30, 0x30, 0x00},   // U+002E (.)
    { 0x06, 0x0C, 0x18, 0x30, 0x60, 0xC0, 0x80, 0x00},   // U+002F (/)
    { 0x7C, 0xC6, 0xCE, 0xDE, 0xF6, 0xE6, 0x7C, 0x00},   // U+0030 (0)
    { 0x30, 0x70, 0x30, 0x30, 0x30, 0x30, 0xFC, 0x00},   // U+0031 (1)
    { 0x78, 0xCC, 0x0C, 0x38, 0x60, 0xCC, 0xFC, 0x00},   // U+0032 (2)
    { 0x78, 0xCC, 0x0C, 0x38, 0x0C, 0xCC, 0x78, 0x00},   // U+0033 (3)
    { 0x1C, 0x3C, 0x6C, 0xCC, 0xFE, 0x0C, 0x1E, 0x00},   // U+0034 (4)
    { 0xFC, 0xC0, 0xF8, 0x0C, 0x0C, 0xCC, 0x78, 0x00},   // U+0035 (5)
    { 0x38, 0x60, 0xC0, 0xF8, 0xCC, 0xCC, 0x78, 0x00},   // U+0036 (6)
    { 0xFC, 0xCC, 0x0C, 0x18, 0x30, 0x30, 0x30, 0x00},   // U+0037 (7)
    { 0x78, 0xCC, 0xCC, 0x78, 0xCC, 0xCC, 0x78, 0x00},   // U+0038 (8)
    { 0x78, 0xCC, 0xCC, 0x7C, 0x0C, 0x18, 0x70, 0x00},   // U+0039 (9)
    { 0x00, 0x30, 0x30, 0x00, 0x00, 0x30, 0x30, 0x00},   // U+003A (:)
    { 0x00, 0x30, 0x30, 0x00, 0x00, 0x30, 0x30, 0x60},   // U+003B (//)
    { 0x18, 0x30, 0x60, 0xC0, 0x60, 0x30, 0x18, 0x00},   // U+003C (<)
    { 0x00, 0x00, 0xFC, 0x00, 0x00, 0xFC, 0x00, 0x00},   // U+003D (=)
    { 0x60, 0x30, 0x18, 0x0C, 0x18, 0x30, 0x60, 0x00},   // U+003E (>)
    { 0x78, 0xCC, 0x0C, 0x18, 0x30, 0x00, 0x30, 0x00},   // U+003F (?)
    { 0x7C, 0xC6, 0xDE, 0xDE, 0xDE, 0xC0, 0x78, 0x00},   // U+0040 (@)
    { 0x30, 0x78, 0xCC, 0xCC, 0xFC, 0xCC, 0xCC, 0x00},   // U+0041 (A)
    { 0xFC, 0x66, 0x66, 0x7C, 0x66, 0x66, 0xFC, 0x00},   // U+0042 (B)
    { 0x3C, 0x66, 0xC0, 0xC0, 0xC0, 0x66, 0x3C, 0x00},   // U+0043 (C)
    { 0xF8, 0x6C, 0x66, 0x66, 0x66, 0x6C, 0xF8, 0x00},   // U+0044 (D)
    { 0xFE, 0x62, 0x68, 0x78, 0x68, 0x62, 0xFE, 0x00},   // U+0045 (E)
    { 0xFE, 0x62, 0x68, 0x78, 0x68, 0x60, 0xF0, 0x00},   // U+0046 (F)
    { 0x3C, 0x66, 0xC0, 0xC0, 0xCE, 0x66, 0x3E, 0x00},   // U+0047 (G)
    { 0xCC, 0xCC, 0xCC, 0xFC, 0xCC, 0xCC, 0xCC, 0x00},   // U+0048 (H)
    { 0x78, 0x30, 0x30, 0x30, 0x30, 0x30, 0x78, 0x00},   // U+0049 (I)
    { 0x1E, 0x0C, 0x0C, 0x0C, 0xCC, 0xCC, 0x78, 0x00},   // U+004A (J)
    { 0xE6, 0x66, 0x6C, 0x78, 0x6C, 0x66, 0xE6, 0x00},   // U+004B (K)
    { 0xF0, 0x60, 0x60, 0x60, 0x62, 0x66, 0xFE, 0x00},   // U+004C (L)
    { 0xC6, 0xEE, 0xFE, 0xFE, 0xD6, 0xC6, 0xC6, 0x00},   // U+004D (M)
    { 0xC6, 0xE6, 0xF6, 0xDE, 0xCE, 0xC6, 0xC6, 0x00},   // U+004E (N)
    { 0x38, 0x6C, 0xC6, 0xC6, 0xC6, 0x6C, 0x38, 0x00},   // U+004F (O)
    { 0xFC, 0x66, 0x66, 0x7C, 0x60, 0x60, 0xF0, 0x00},   // U+0050 (P)
    { 0x78, 0xCC, 0xCC, 0xCC, 0xDC, 0x78, 0x1C, 0x00},   // U+0051 (Q)
    { 0xFC, 0x66, 0x66, 0x7C, 0x6C, 0x66, 0xE6, 0x00},   // U+0052 (R)
    { 0x78, 0xCC, 0xE0, 0x70, 0x1C, 0xCC, 0x78, 0x00},   // U+0053 (S)
    { 0xFC, 0xB4, 0x30, 0x30, 0x30, 0x30, 0x78, 0x00},   // U+0054 (T)
    { 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0xFC, 0x00},   // U+0055 (U)
    { 0xCC, 0xCC, 0xCC, 0xCC, 0xCC, 0x78, 0x30, 0x00},   // U+0056 (V)
    { 0xC6, 0xC6, 0xC6, 0xD6, 0xFE, 0xEE, 0xC6, 0x00},   // U+0057 (W)
    { 0xC6, 0xC6, 0x6C, 0x38, 0x38, 0x6C, 0xC6, 0x00},   // U+0058 (X)
    { 0xCC, 0xCC, 0xCC, 0x78, 0x30, 0x30, 0x78, 0x00},   // U+0059 (Y)
    { 0xFE, 0xC6, 0x8C, 0x18, 0x32, 0x66, 0xFE, 0x00},   // U+005A (Z)
    { 0x78, 0x60, 0x60, 0x60, 0x60, 0x60, 0x78, 0x00},   // U+005B ([)
    { 0xC0, 0x60, 0x30, 0x18, 0x0C, 0x06, 0x02, 0x00},   // U+005C (\)
    { 0x78, 0x18, 0x18, 0x18, 0x18, 0x18, 0x78, 0x00},   // U+005D (])
    { 0x10, 0x38, 0x6C, 0xC6, 0x00, 0x00, 0x00, 0x00},   // U+005E (^)
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF},   // U+005F (_)
    { 0x30, 0x30, 0x18, 0x00, 0x00, 0x00, 0x00, 0x00},   // U+0060 (`)
    { 0x00, 0x00, 0x78, 0x0C, 0x7C, 0xCC, 0x76, 0x00},   // U+0061 (a)
    { 0xE0, 0x60, 0x60, 0x7C, 0x66, 0x66, 0xDC, 0x00},   // U+0062 (b)
    { 0x00, 0x00, 0x78, 0xCC, 0xC0, 0xCC, 0x78, 0x00},   // U+0063 (c)
    { 0x1C, 0x0C, 0x0C, 0x7C, 0xCC, 0xCC, 0x76, 0x00},   // U+0064 (d)
    { 0x00, 0x00, 0x78, 0xCC, 0xFC, 0xC0, 0x78, 0x00},   // U+0065 (e)
    { 0x38, 0x6C, 0x60, 0xF0, 0x60, 0x60, 0xF0, 0x00},   // U+0066 (f)
    { 0x00, 0x00, 0x76, 0xCC, 0xCC, 0x7C, 0x0C, 0xF8},   // U+0067 (g)
    { 0xE0, 0x60, 0x6C, 0x76, 0x66, 0x66, 0xE6, 0x00},   // U+0068 (h)
    { 0x30, 0x00, 0x70, 0x30, 0x30, 0x30, 0x78, 0x00},   // U+0069 (i)
    { 0x0C, 0x00, 0x0C, 0x0C, 0x0C, 0xCC, 0xCC, 0x78},   // U+006A (j)
    { 0xE0, 0x60, 0x66, 0x6C, 0x78, 0x6C, 0xE6, 0x00},   // U+006B (k)
    { 0x70, 0x30, 0x30, 0x30, 0x30, 0x30, 0x78, 0x00},   // U+006C (l)
    { 0x00, 0x00, 0xCC, 0xFE, 0xFE, 0xD6, 0xC6, 0x00},   // U+006D (m)
    { 0x00, 0x00, 0xF8, 0xCC, 0xCC, 0xCC, 0xCC, 0x00},   // U+006E (n)
    { 0x00, 0x00, 0x78, 0xCC, 0xCC, 0xCC, 0x78, 0x00},   // U+006F (o)
    { 0x00, 0x00, 0xDC, 0x66, 0x66, 0x7C, 0x60, 0xF0},   // U+0070 (p)
    { 0x00, 0x00, 0x76, 0xCC, 0xCC, 0x7C, 0x0C, 0x1E},   // U+0071 (q)
    { 0x00, 0x00, 0xDC, 0x76, 0x66, 0x60, 0xF0, 0x00},   // U+0072 (r)
    { 0x00, 0x00, 0x7C, 0xC0, 0x78, 0x0C, 0xF8, 0x00},   // U+0073 (s)
    { 0x10, 0x30, 0x7C, 0x30, 0x30, 0x34, 0x18, 0x00},   // U+0074 (t)
    { 0x00, 0x00, 0xCC, 0xCC, 0xCC, 0xCC, 0x76, 0x00},   // U+0075 (u)
    { 0x00, 0x00, 0xCC, 0xCC, 0xCC, 0x78, 0x30, 0x00},   // U+0076 (v)
    { 0x00, 0x00, 0xC6, 0xD6, 0xFE, 0xFE, 0x6C, 0x00},   // U+0077 (w)
    { 0x00, 0x00, 0xC6, 0x6C, 0x38, 0x6C, 0xC6, 0x00},   // U+0078 (x)
    { 0x00, 0x00, 0xCC, 0xCC, 0xCC, 0x7C, 0x0C, 0xF8},   // U+0079 (y)
    { 0x00, 0x00, 0xFC, 0x98, 0x30, 0x64, 0xFC, 0x00},   // U+007A (z)
    { 0x1C, 0x30, 0x30, 0xE0, 0x30, 0x30, 0x1C, 0x00},   // U+007B ({)
    { 0x18, 0x18, 0x18, 0x00, 0x18, 0x18, 0x18, 0x00},   // U+007C (|)
    { 0xE0, 0x30, 0x30, 0x1C, 0x30, 0x30, 0xE0, 0x00},   // U+007D (})
    { 0x76, 0xDC, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},   // U+007E (~)
    { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}    // U+007F
};

/*  Game of Life program  */
#define GAME_HEIGHT 60
#define GAME_WIDTH 80
int board[GAME_HEIGHT][GAME_WIDTH];
int cursor_x = 15; 
int cursor_y = 15;

void delay() {
  for (int i = 0; i < DELAY; i++) {
    sys_set_frame(0);
  }
}

void life_init() {
  for (int i = 0; i < GAME_HEIGHT; i++) {
    for (int j = 0; j < GAME_WIDTH; j++) {
      board[i][j] = 0;
    }
  }
  //for now, add some squares
  board[11][21] = 1;
  board[11][22] = 1;
  board[11][23] = 1;
  board[12][21] = 1;
  board[13][22] = 1;

  board[30][40] = 1;
  board[30][41] = 1;
  board[30][42] = 1;
  board[30][43] = 1;
  board[30][44] = 1;
  board[30][45] = 1;
  board[30][46] = 1;
  board[30][47] = 1;
  board[30][48] = 1;
  board[30][49] = 1;

  //draw initial board
  for (int y = 0; y < GAME_HEIGHT; y++) {  
    for (int x = 0; x < GAME_WIDTH; x++) {   
      color_xy(x, y, (board[y][x] ? 0xFF : 0x00), 0);
    }
  }
}

void color_xy(int x, int y, char value, int plane) {
  sys_set_frame(plane);
  for(int i = 0; i < 8; i++){
    vga_mem[(y*8 + i - 1) * GAME_WIDTH + x] = value;
  }
}

void color_xy_font(int x, int y, char * letter, int plane) {
  sys_set_frame(plane);
  for(int i = 0; i < 8; i++){
    vga_mem[(y*8 + i - 1) * GAME_WIDTH + x] = letter[i];
  }
}

void print_at_point(int x, int y, char * string, int plane) {
  int i = 0;
  while (string[i] != '\0') {
    color_xy_font(x, y, font8x8_basic[string[i]], plane);
    i++;
    x++;
  }
}

void evolve_and_print() {
  int counts[GAME_HEIGHT][GAME_WIDTH];
  int new[GAME_HEIGHT][GAME_WIDTH];
  for (int y = 0; y < GAME_HEIGHT; y++) {  
    for (int x = 0; x < GAME_WIDTH; x++) {
      counts[y][x] = 0;
      //check adjacent squares
      for (int cur_y = y - 1; cur_y <= y + 1; cur_y++) {
        for (int cur_x = x - 1; cur_x <= x + 1; cur_x++) {
          if (y != cur_y || x != cur_x) {
            counts[y][x] += board[(cur_y + GAME_HEIGHT) % GAME_HEIGHT][(cur_x + GAME_WIDTH) % GAME_WIDTH];
          }
        }
      }
      //determine future
      if (board[y][x]) {
        if (counts[y][x] == 2 || counts[y][x] == 3) {
          new[y][x] = 1;
        } else {
          new[y][x] = 0;
        }
      } else {
        if (counts[y][x] == 3) {
          new[y][x] = 1;
        } else {
          new[y][x] = 0;
        }
      }
    }
  }
  //copy over
  for (int y = 0; y < GAME_HEIGHT; y++) {  
    for (int x = 0; x < GAME_WIDTH; x++) {
      if (board[y][x] != new[y][x]) {
        color_xy(x, y, (new[y][x] ? 0xFF : 0x00), 0);
      }      
      board[y][x] = new[y][x];
    }
  }
}

void print_help() {
  print_at_point(4, 2, "Welcome to Conway's Game of Life! Use the WASD Keys to Move the Cursor.", 2);
  print_at_point(4, 4, "Press P to place cells, R to remove cells, and T to toggle cells.", 2);
  print_at_point(4, 6, "Press G to start the simulation and B to pause it and make changes.", 2);
  print_at_point(4, 8, "End the game from this screen with B, and toggle this Help Menu with H", 2);
  print_at_point(4, 10, "Enjoy!", 2);
}


void life_setup() {
  int key;
  int blink = 0;
  int state = 1;
  int help = 1;
  color_xy(cursor_x, cursor_y, 0xFF, 3);
  //text
  print_help();

  while (1) {
    key = sys_get_keyboard(0);
    if (key == 'a') {
       color_xy(cursor_x, cursor_y, 0x00, 3);
      cursor_x = (cursor_x - 1 + GAME_WIDTH) % GAME_WIDTH;
      color_xy(cursor_x, cursor_y, (blink < BLINK_DELAY/2 ? 0xFF: 0x00), 3);
    } else if (key == 'd') {
       color_xy(cursor_x, cursor_y, 0x00, 3);
      cursor_x = (cursor_x + 1) % GAME_WIDTH;
      color_xy(cursor_x, cursor_y, (blink < BLINK_DELAY/2 ? 0xFF: 0x00), 3);
    } else if (key == 'w') {
       color_xy(cursor_x, cursor_y, 0x00, 3);
      cursor_y = (cursor_y - 1 + GAME_HEIGHT) % GAME_HEIGHT;
      color_xy(cursor_x, cursor_y, (blink < BLINK_DELAY/2 ? 0xFF: 0x00), 3);
    } else if (key == 's') { 
     color_xy(cursor_x, cursor_y, 0x00, 3);   
      cursor_y = (cursor_y  + 1) % GAME_HEIGHT;
      color_xy(cursor_x, cursor_y, (blink < BLINK_DELAY/2 ? 0xFF: 0x00), 3);
    } else if (key == 'b') {
      sys_switch_mode(0);
      return;
    } else if (key == 'p') {
      board[cursor_y][cursor_x] = 1;
      color_xy(cursor_x, cursor_y, 0xFF, 0);
    } else if (key == 't') {
      board[cursor_y][cursor_x] = (board[cursor_y][cursor_x] ? 0 : 1);
      color_xy(cursor_x, cursor_y, (board[cursor_y][cursor_x] ? 0xFF : 0x00), 0);
    } else if (key == 'r') {
       board[cursor_y][cursor_x] = 0;
        color_xy(cursor_x, cursor_y, 0x00, 0);
    } else if (key == 'g') {
      if (help) {
         vga_set_frame(2);
         for(int i = 0; i < MAP_SIZE/4; i++){
            vga_mem[i] = 0x00;
          }
          help = 0;
      }
      color_xy(cursor_x, cursor_y, 0x00, 3);
      life_continuous_evolve();
      color_xy(cursor_x, cursor_y, 0xFF, 3);
    } else if (key == 'h') {
      if (help) {
         vga_set_frame(2);
         for(int i = 0; i < MAP_SIZE/4; i++){
            vga_mem[i] = 0x00;
          }
          help = 0;
      } else {
        print_help();
        help = 1;
      }
    }

    blink = (blink > BLINK_DELAY ? 0: blink + 1);
    if (!blink && state == 0) {
      color_xy(cursor_x, cursor_y, 0xFF, 3);
      state = 1;
    } else if (blink >= BLINK_DELAY/2 && state == 1) {
      state = 0;
      color_xy(cursor_x, cursor_y, 0x00, 3);
    }
  }
}

void life_continuous_evolve() {
  int key;
  while(1) {
    evolve_and_print(); 
    delay();
    if ((key = sys_get_keyboard(0)) != 0) {
      //printf("got: %d\n", key);
      if (key == (int) 'b') {
        return;
      }
    }
  }
}

void life() {
  sys_switch_mode(1);
  life_init();
  life_setup();
}

/* file system */

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
    printf("Setting up VGA map\n");
    vga_init();

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
      } else if (!strcmp(arg_array[0], "novideo")){
        sys_switch_mode(0);
      }else if (!strcmp(arg_array[0], "draw2")) {
        vga_set_frame(2);
        for(int i = 0; i < MAP_SIZE; i++){
          vga_mem[i] = 0xFF;
        }
        vga_set_frame(0);
      }else if (!strcmp(arg_array[0], "draw3")) {
        vga_set_frame(3);
        for(int i = 0; i < MAP_SIZE; i++){
          vga_mem[i] = 0xFF;
        }
        vga_set_frame(0);
      }else if (!strcmp(arg_array[0], "draw1")) {
        vga_set_frame(1);
        for(int i = 0; i < MAP_SIZE; i++){
          vga_mem[i] = 0xFF;
        }
        vga_set_frame(0);
      }else if (!strcmp(arg_array[0], "draw0")) {
        vga_set_frame(0);
        for(int i = 0; i < MAP_SIZE; i++){
          vga_mem[i] = 0xFF;
        }
        vga_set_frame(0);
      }else if (!strcmp(arg_array[0], "draw_clear_all")) {
        for (int frame = 0; frame < 4; frame++) {
          vga_set_frame(frame);
           for(int i = 0; i < MAP_SIZE; i++){
              vga_mem[i] = 0x00;
            }
        }
        vga_set_frame(0);
      }else if (!strcmp(arg_array[0], "life")) {
        life();
      }else {
        // TODO more here!
        printf("unrecognized command: %s\n", arg_array[0]);
         
      }
      linebuf[0] = '\0';
    }
    return 0;
}
