#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "ttykit.h"
#include "buffer.h"

int main(void) {
    if (tty_enable_raw_mode() == -1) {
        perror("tty_enable_raw_mode");
        return 1;
    }

    tty_enter_alternate_screen();
    tty_cursor_hide();

    int rows, cols;
    tty_get_size(&rows, &cols);

    Buffer *buf = buffer_create(rows, cols);
    if (!buf) {
        tty_leave_alternate_screen();
        tty_disable_raw_mode();
        return 1;
    }

    // Draw centered text
    const char *msg = "Hello, ttykit!";
    int len = strlen(msg);
    buffer_set_str(buf, rows / 2, (cols - len) / 2, msg);

    // Draw border
    for (int c = 0; c < cols; c++) {
        buffer_set_cell(buf, 0, c, '-');
        buffer_set_cell(buf, rows - 1, c, '-');
    }
    for (int r = 0; r < rows; r++) {
        buffer_set_cell(buf, r, 0, '|');
        buffer_set_cell(buf, r, cols - 1, '|');
    }

    // Show info at bottom
    char info[64];
    snprintf(info, sizeof(info), " %dx%d - Press 'q' to quit ", cols, rows);
    buffer_set_str(buf, rows - 1, 2, info);

    buffer_render(buf);

    char c;
    while (read(STDIN_FILENO, &c, 1) == 1) {
        if (c == 'q') break;
    }

    buffer_destroy(buf);
    tty_cursor_show();
    tty_leave_alternate_screen();
    tty_disable_raw_mode();
    return 0;
}
