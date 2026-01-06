#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "ttykit.h"

int main(void) {
    if (tty_enable_raw_mode() == -1) {
        perror("tty_enable_raw_mode");
        return 1;
    }

    tty_enter_alternate_screen();
    tty_cursor_hide();
    tty_clear_screen();

    int rows, cols;
    tty_get_size(&rows, &cols);

    // Draw centered text
    const char *msg = "Hello, ttykit!";
    int len = strlen(msg);
    int row = rows / 2;
    int col = (cols - len) / 2;

    tty_cursor_move(row, col);
    write(STDOUT_FILENO, msg, len);

    // Show size info at bottom
    char buf[64];
    int n = snprintf(buf, sizeof(buf), "Screen: %dx%d  Press 'q' to quit", cols, rows);
    tty_cursor_move(rows, 1);
    write(STDOUT_FILENO, buf, n);

    char c;
    while (read(STDIN_FILENO, &c, 1) == 1) {
        if (c == 'q') break;
    }

    tty_cursor_show();
    tty_leave_alternate_screen();
    tty_disable_raw_mode();
    return 0;
}
