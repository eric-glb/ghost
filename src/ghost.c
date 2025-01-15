#include "include/ghost.h"
#include "include/frames.h"

struct termios orig_termios;
volatile sig_atomic_t last_frame_index = 0;

char *buffer;
char formatted_frames
    [FRAME_COUNT]
    [IMAGE_HEIGHT]
    [MAX_LINE_LENGTH * 2];

int term_rows, term_cols;
int start_row, start_col;

long long get_microseconds(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (long long)ts.tv_sec * 1000000LL + ts.tv_nsec / 1000;
}

void get_terminal_size(int *rows, int *cols) {
    struct winsize w;
    ioctl(STDOUT_FILENO, TIOCGWINSZ, &w);
    *rows = w.ws_row;
    *cols = w.ws_col;
}

void enable_raw_mode(void) {
    tcgetattr(STDIN_FILENO, &orig_termios);
    struct termios raw = orig_termios;
    raw.c_lflag &= ~(ECHO | ICANON);
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
}

void disable_raw_mode(void) {
    tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
}

int kbhit(void) {
    int oldf = fcntl(STDIN_FILENO, F_GETFL, 0);
    fcntl(STDIN_FILENO, F_SETFL, oldf | O_NONBLOCK);
    int ch = getchar();
    fcntl(STDIN_FILENO, F_SETFL, oldf);
    if (ch != EOF) {
        ungetc(ch, stdin);
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

void move_cursor(int row, int col) {
    char buffer[32];
    char *ptr = buffer;
    
    *ptr++ = '\x1b';
    *ptr++ = '[';
    
    if (row >= 100) *ptr++ = (row / 100) + '0';
    if (row >= 10) *ptr++ = ((row / 10) % 10) + '0';
    *ptr++ = (row % 10) + '0';
    
    *ptr++ = ';';
    
    if (col >= 100) *ptr++ = (col / 100) + '0';
    if (col >= 10) *ptr++ = ((col / 10) % 10) + '0';
    *ptr++ = (col % 10) + '0';
    
    *ptr++ = 'H';
    *ptr = '\0';
    
    write(STDOUT_FILENO, buffer, ptr - buffer);
}

void clear_line_to_end(void) {
    CSI(ERASE_LINE);
}

void update_dimensions(void) {
    get_terminal_size(&term_rows, &term_cols);

    int vertical_padding = (term_rows - IMAGE_HEIGHT) / 2;
    int horizontal_padding = (term_cols - IMAGE_WIDTH) / 2;

    start_row = (vertical_padding >= 0) ? vertical_padding : 0;
    start_col = (horizontal_padding >= 0) ? horizontal_padding : 0;
}

void clear_screen(void) {
    CSI(CLEAR_SCREEN);
    CSI(MOVE_CURSOR_HOME);
    fflush(stdout);
}

void prepare_terminal(void) {
    enable_raw_mode();
    CSI(ALTERNATE_SCREEN);
    CSI(CLEAR_SCREEN);
    CSI(CURSOR_HIDE);
    fflush(stdout);
}

void restore_terminal(void) {
    CSI(CURSOR_SHOW);
    CSI(MAIN_SCREEN);
    disable_raw_mode();
    fflush(stdout);
}

void handle_resize(int sig) {
    update_dimensions();

    if (term_cols < 115 || term_rows < 56) {
        clear_screen();
        restore_terminal();
        disable_raw_mode();
        exit(EXIT_FAILURE);
    }

    clear_screen();
    last_frame_index = -1;
}

void handle_sigint(int sig) {
    for (int i = 0; i < IMAGE_HEIGHT; i++) {
        move_cursor(start_row + i, 1);
        clear_line_to_end();
    }

    free(buffer);
    restore_terminal();
    exit(EXIT_SUCCESS);
}

void preformat_frames(void) {
    for (size_t frame = 0; frame < FRAME_COUNT; frame++) {
        for (int i = 0; i < IMAGE_HEIGHT; i++) {
            const char *line = animation_frames[frame][i];
            char *output = formatted_frames[frame][i];

            const char *ptr = line;
            while (*ptr) {
                if (strncmp(ptr, "<color>", 7) == 0) {
                    memcpy(output, COLOR_BLUE, strlen(COLOR_BLUE));
                    output += strlen(COLOR_BLUE);
                    ptr += 7;
                }
                else if (strncmp(ptr, "</color>", 8) == 0) {
                    memcpy(output, COLOR_RESET, strlen(COLOR_RESET));
                    output += strlen(COLOR_RESET);
                    ptr += 8;
                }
                else {
                    *output++ = *ptr++;
                }
            }
            *output = '\0';
        }
    }
}

int main(void) {
    struct sigaction sa;
    memset(&sa, 0, sizeof(struct sigaction));
    sa.sa_handler = handle_resize;
    
    sigaction(SIGWINCH, &sa, NULL);
    signal(SIGINT, handle_sigint);

    get_terminal_size(&term_rows, &term_cols);
    if (term_cols < 115 || term_rows < 56) {
        fprintf(stderr,
            "Terminal size too small. Minimum "
            "required: %dx%d, Current: %dx%d\n",
            115, 56, term_cols, term_rows
        );
        return EXIT_FAILURE;
    }

    prepare_terminal();
    setvbuf(stdout, NULL, _IOFBF, 0);

    buffer = malloc(IMAGE_HEIGHT * (term_cols + 1));
    update_dimensions();
    preformat_frames();

    long long start_time = get_microseconds();
    long long next_frame_time = start_time + MICROS_PER_FRAME;

    while (1) {
        long long current_time = get_microseconds();
        size_t frame_index = (current_time - start_time) / MICROS_PER_FRAME % FRAME_COUNT;

        if (frame_index != last_frame_index) {
            for (int i = 0; i < IMAGE_HEIGHT; i++) {
                char *line = buffer + i * (term_cols + 1);
                memset(line, ' ', term_cols);
                line[term_cols] = '\0';

                memcpy(line + start_col, 
                    formatted_frames[frame_index][i], 
                    strlen(formatted_frames[frame_index][i])
                );

                move_cursor(start_row + i, 1);
                write(STDOUT_FILENO, line, term_cols);
                clear_line_to_end();
            }

            last_frame_index = frame_index;
        }

        if (kbhit()) {
            char c = getchar();
            if (c == 'q' || c == 'Q')
                break;
        }

        long long sleep_time = next_frame_time - get_microseconds();
        if (sleep_time > 0) {
            struct timespec ts = {0, sleep_time * 1000};
            nanosleep(&ts, NULL);
        }
        next_frame_time += MICROS_PER_FRAME;
    }

    for (int i = 0; i < IMAGE_HEIGHT; i++) {
        move_cursor(start_row + i, 1);
        clear_line_to_end();
    }

    free(buffer);
    restore_terminal();
    return EXIT_SUCCESS;
}
