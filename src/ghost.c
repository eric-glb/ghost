#include <signal.h>

#include "include/ghost.h"
#include "include/frames.h"

struct termios orig_termios;
volatile sig_atomic_t resize_needed = 0;

char **current_buffer;
char **next_buffer;

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
    printf("%s[%d;%dH", ESC, row, col);
}

char **create_buffer(int height, int width) {
    char **buffer = malloc(height * sizeof(char *));
    for (int i = 0; i < height; i++) {
        buffer[i] = malloc(width);
        memset(buffer[i], ' ', width - 1);
        buffer[i][width - 1] = '\0';
    }
    return buffer;
}

void free_buffer(char **buffer, int height) {
    for (int i = 0; i < height; i++) {
        free(buffer[i]);
    }
    free(buffer);
}

void clear_line_to_end(void) {
    printf("%s[K", ESC);
}

void handle_resize(int sig) {
    resize_needed = 1;
}

void update_dimensions(void) {
    get_terminal_size(&term_rows, &term_cols);

    start_row = (term_rows - IMAGE_HEIGHT) / 2;
    start_col = (term_cols - IMAGE_WIDTH) / 2;
}

void clear_screen(void) {
    printf("%s[2J", ESC); // clear entire screen
    printf("%s[H", ESC);  // move cursor to home position
    fflush(stdout);
}

void restore_terminal(void) {
    printf("%s[?25h", ESC);   // show cursor
    printf("%s[?1049l", ESC); // main screen
    disable_raw_mode();
}

void handle_sigint(int sig) {
    for (int i = 0; i < IMAGE_HEIGHT; i++) {
        move_cursor(start_row + i, 1);
        clear_line_to_end();
    }

    free_buffer(current_buffer, IMAGE_HEIGHT);
    free_buffer(next_buffer, IMAGE_HEIGHT);

    restore_terminal();
    exit(EXIT_SUCCESS);
}

int main(void) {
    struct sigaction sa;
    memset(&sa, 0, sizeof(struct sigaction));
    sa.sa_handler = handle_resize;
    
    sigaction(SIGWINCH, &sa, NULL);
    signal(SIGINT, handle_sigint);

    get_terminal_size(&term_rows, &term_cols);
    if (term_rows < IMAGE_HEIGHT + 8 || term_cols < IMAGE_WIDTH) {
        fprintf(stderr,
            "Terminal size too small. Minimum "
            "required: %dx%d, Current: %dx%d\n",
            IMAGE_WIDTH + 8, IMAGE_HEIGHT + 8, term_cols, term_rows
        );
        return EXIT_FAILURE;
    }

    enable_raw_mode();
    printf("%s[?1049h", ESC); // alternate screen
    printf("%s[2J", ESC);     // clear screen
    printf("%s[?25l", ESC);   // hide cursor
    fflush(stdout);

    setvbuf(stdout, NULL, _IOFBF, 0);

    current_buffer = create_buffer(IMAGE_HEIGHT, term_cols + 1);
    next_buffer = create_buffer(IMAGE_HEIGHT, term_cols + 1);

    update_dimensions();

    long long start_time = get_microseconds();
    size_t last_frame_index = 0;

    while (1) {
        if (resize_needed) {
            int old_cols = term_cols;

            update_dimensions();

            if (term_rows < IMAGE_HEIGHT + 8 || term_cols < IMAGE_WIDTH) {
                printf("%s[2J", ESC);     // clear screen
                printf("%s[H", ESC);      // move to home position
                printf("%s[?25h", ESC);   // show cursor
                printf("%s[?1049l", ESC); // main screen
                disable_raw_mode();
                exit(EXIT_FAILURE);
            }

            if (term_cols != old_cols) {
                free_buffer(current_buffer, IMAGE_HEIGHT);
                free_buffer(next_buffer, IMAGE_HEIGHT);
                current_buffer = create_buffer(IMAGE_HEIGHT, term_cols + 1);
                next_buffer = create_buffer(IMAGE_HEIGHT, term_cols + 1);
            }

            clear_screen();
            last_frame_index = -1;
            resize_needed = 0;
        }

        long long elapsed = get_microseconds() - start_time;
        size_t frame_index = (elapsed / MICROS_PER_FRAME) % FRAME_COUNT;

        if (frame_index != last_frame_index) {
            for (int i = 0; i < IMAGE_HEIGHT; i++) {
                memset(next_buffer[i], ' ', term_cols);
                next_buffer[i][term_cols] = '\0';
            }

            for (int i = 0; i < IMAGE_HEIGHT; i++) {
                const char *line = animation_frames[frame_index][i];
                char formatted_line[MAX_LINE_LENGTH * 2] = {0};
                char *output = formatted_line;

                const char *ptr = line;
                while (*ptr) {
                    if (strncmp(ptr, "<color>", 7) == 0) {
                        strcpy(output, COLOR_BLUE);
                        output += strlen(COLOR_BLUE);
                        ptr += 7;
                    }
                    else if (strncmp(ptr, "</color>", 8) == 0) {
                        strcpy(output, COLOR_RESET);
                        output += strlen(COLOR_RESET);
                        ptr += 8;
                    }
                    else {
                        *output++ = *ptr++;
                    }
                }
                *output = '\0';

                char *buffer_ptr = next_buffer[i] + start_col;
                strcpy(buffer_ptr, formatted_line);
            }

            for (int i = 0; i < IMAGE_HEIGHT; i++) {
                if (strcmp(current_buffer[i], next_buffer[i]) != 0) {
                    move_cursor(start_row + i, 1);
                    printf("%s", next_buffer[i]);
                    clear_line_to_end();
                    strcpy(current_buffer[i], next_buffer[i]);
                }
            }

            fflush(stdout);
            last_frame_index = frame_index;
        }

        if (kbhit()) {
            char c = getchar();
            if (c == 'q' || c == 'Q')
                break;
        }

        usleep(MICROS_PER_FRAME / 2);
    }

    for (int i = 0; i < IMAGE_HEIGHT; i++) {
        move_cursor(start_row + i, 1);
        clear_line_to_end();
    }

    free_buffer(current_buffer, IMAGE_HEIGHT);
    free_buffer(next_buffer, IMAGE_HEIGHT);

    restore_terminal();
    return EXIT_SUCCESS;
}
