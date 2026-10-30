#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_INPUT_LEN 1024
#define PROMPT ""
#define PROMPT_LEN (sizeof(PROMPT) - 1)

/* --- Cross-Platform Console Setup & Terminal Width Detection --- */
#if defined(_WIN32) || defined(_WIN64)
#include <conio.h>
#include <windows.h>

void set_raw_mode(int enable) { (void)enable; }
int get_char(void) { return _getch(); }
int has_key_waiting(void) { return _kbhit(); }

int get_terminal_width(void)
{
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    if (GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi)) {
        return csbi.srWindow.Right - csbi.srWindow.Left + 1;
    }
    return 80;
}
#else
#include <sys/ioctl.h>
#include <sys/select.h>
#include <termios.h>
#include <unistd.h>

static struct termios orig_termios;

void set_raw_mode(int enable)
{
    if (enable) {
        tcgetattr(STDIN_FILENO, &orig_termios);
        struct termios raw = orig_termios;
        raw.c_lflag &= ~(ICANON | ECHO);
        tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw);
    } else {
        tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
    }
}

int get_char(void)
{
#if defined(_WIN32) || defined(_WIN64)
    return getchar();
#else
    unsigned char c;
    ssize_t n = read(STDIN_FILENO, &c, 1);
    return (n == 1) ? c : -1;
#endif
}

int has_key_waiting(void)
{
    struct timeval tv = { 0, 0 };
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(STDIN_FILENO, &fds);
    return select(STDIN_FILENO + 1, &fds, NULL, NULL, &tv) > 0;
}

int get_terminal_width(void)
{
    struct winsize ws;
    if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0 && ws.ws_col > 0) {
        return ws.ws_col;
    }
    return 80;
}
#endif

enum KeyCode {
    KEY_NONE = 0,
    KEY_LEFT,
    KEY_RIGHT,
    KEY_UP,
    KEY_DOWN,
    KEY_BACKSPACE,
    KEY_DELETE,
    KEY_ENTER,
    KEY_ESC,
    KEY_CHAR
};

int read_key(int* out_char)
{
    int ch = get_char();

    if (ch == '\r' || ch == '\n')
        return KEY_ENTER;
    if (ch == 3)
        return KEY_ESC;
    if (ch == 127 || ch == 8)
        return KEY_BACKSPACE;

#if defined(_WIN32) || defined(_WIN64)
    if (ch == 0 || ch == 224) {
        int extended = get_char();
        switch (extended) {
        case 75:
            return KEY_LEFT;
        case 77:
            return KEY_RIGHT;
        case 72:
            return KEY_UP;
        case 80:
            return KEY_DOWN;
        case 83:
            return KEY_DELETE;
        default:
            return KEY_NONE;
        }
    }
    if (ch == 27)
        return KEY_ESC;
#else
    if (ch == 27) {
        if (!has_key_waiting())
            return KEY_ESC;

        int next1 = get_char();
        if (next1 == '[') {
            int next2 = get_char();
            switch (next2) {
            case 'D':
                return KEY_LEFT;
            case 'C':
                return KEY_RIGHT;
            case 'A':
                return KEY_UP;
            case 'B':
                return KEY_DOWN;
            case '3':
                if (has_key_waiting() && get_char() == '~') {
                    return KEY_DELETE;
                }
                return KEY_NONE;
            default:
                return KEY_NONE;
            }
        }
        return KEY_NONE;
    }
#endif

    if (ch >= 32 && ch <= 126) {
        *out_char = ch;
        return KEY_CHAR;
    }

    return KEY_NONE;
}

void lex(const char* str);

int main(void)
{
    // -1-(-4)
    // a+
    // a+4-b
    // f-4
    // a+b
    // a-(b+c)
    char input[MAX_INPUT_LEN] = "3.333 + a";
#if defined(_DEBUG)

    printf("Exp: %s\n", input);
    lex(input);
    return 0;
#endif

    size_t len = strlen(input);
    size_t cursor_pos = len;

    int prev_input_rows = 0;

    set_raw_mode(1);

    printf("Type expression:\n");

    while (1) {
        int term_width = get_terminal_width();
        if (term_width <= 0)
            term_width = 80;

        int total_input_chars = PROMPT_LEN + (int)len;
        int current_input_rows = (total_input_chars == 0) ? 0 : (total_input_chars - 1) / term_width;

        printf("\033[?25l"); // hide cursor
        if (prev_input_rows > 0) //  move up ONLY by the visual rows occupied by the input text itself
            printf("\033[%dA", prev_input_rows);

        printf("\r\033[J"); //            clear from start of Input line to the bottom of terminal screen
        printf("%s%s", PROMPT, input); // render prompt and input
        printf("\033[s\n"); //            save cursor position RIGHT HERE (at end of input text)
        lex(input); //           render words on individual lines below
        printf("\033[u"); //              restore cursor directly to the end of the input line

        // reposition cursor within wrapped input line to match cursor_pos
        int cursor_char_offset = PROMPT_LEN + (int)cursor_pos;
        int target_cursor_row = cursor_char_offset / term_width;
        int target_cursor_col = (cursor_char_offset % term_width) + 1; // 1-based index

        int move_up_within_input = current_input_rows - target_cursor_row;
        if (move_up_within_input > 0)
            printf("\033[%dA", move_up_within_input);

        printf("\033[%dG", target_cursor_col);
        printf("\033[?25h"); // show cursor
        fflush(stdout);

        prev_input_rows = current_input_rows;

        int character = 0;
        int key = read_key(&character);

        if (key == KEY_ENTER || key == KEY_ESC)
            break;

        if (key == KEY_LEFT) {
            if (cursor_pos > 0)
                cursor_pos--;
        } else if (key == KEY_RIGHT) {
            if (cursor_pos < len)
                cursor_pos++;
        } else if (key == KEY_UP) {
            cursor_pos = 0;
        } else if (key == KEY_DOWN) {
            cursor_pos = len;
        } else if (key == KEY_BACKSPACE) {
            if (cursor_pos > 0) {
                memmove(&input[cursor_pos - 1], &input[cursor_pos], len - cursor_pos + 1);
                len--;
                cursor_pos--;
            }
        } else if (key == KEY_DELETE) {
            if (cursor_pos < len) {
                memmove(&input[cursor_pos], &input[cursor_pos + 1], len - cursor_pos);
                len--;
            }
        } else if (key == KEY_CHAR) {
            if (len < MAX_INPUT_LEN - 1) {
                memmove(&input[cursor_pos + 1], &input[cursor_pos], len - cursor_pos + 1);
                input[cursor_pos] = (char)character;
                len++;
                cursor_pos++;
            }
        }
    }

    printf("\033[u\033[J\n");
    set_raw_mode(0);
    printf("Done.\n");
    return 0;
}
