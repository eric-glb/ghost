#ifndef ANIMATION_H
#define ANIMATION_H

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>

#define MAX_LINE_LENGTH 256
#define MICROS_PER_FRAME 30000
#define CSI(str) fputs(str, stdout)

#define COLOR_RESET "\x1b[0m"
#define COLOR_BLUE "\x1b[34m"

#define CURSOR_SHOW "\x1b[?25h"
#define CURSOR_HIDE "\x1b[?25l"
#define CLEAR_SCREEN "\x1b[2J"
#define ERASE_LINE "\x1b[K"
#define MOVE_CURSOR_HOME "\x1b[H"
#define ALTERNATE_SCREEN "\x1b[?1049h"
#define MAIN_SCREEN "\x1b[?1049l"

long long get_microseconds(void);
void get_terminal_size(int *rows, int *cols);
void enable_raw_mode(void);
void disable_raw_mode(void);
int kbhit(void);
void move_cursor(int row, int col);
char **create_buffer(int height, int width);
void free_buffer(char **buffer, int height);
void clear_line_to_end(void);
void update_dimensions(void);
void clear_screen(void);
void prepare_terminal(void);
void restore_terminal(void);
void handle_resize(int sig);
void handle_sigint(int sig);
void preformat_frames(void);

struct termios orig_termios;

struct timespec frame_ts = {
    .tv_sec = 0,
    .tv_nsec = (MICROS_PER_FRAME / 2) * 1000
};

#endif
