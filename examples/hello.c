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

    // Draw at specific positions
    const char *s1 = "Top-left corner";
    const char *s2 = "Row 5, Col 10";
    const char *s3 = "Press 'q' to quit";

    tty_cursor_move(1, 1);
    write(STDOUT_FILENO, s1, strlen(s1));

    tty_cursor_move(5, 10);
    write(STDOUT_FILENO, s2, strlen(s2));

    tty_cursor_move(10, 20);
    write(STDOUT_FILENO, s3, strlen(s3));

    char c;
    while (read(STDIN_FILENO, &c, 1) == 1) {
        if (c == 'q') break;
    }

    tty_cursor_show();
    tty_leave_alternate_screen();
    tty_disable_raw_mode();
    return 0;
}
