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

    // Draw centered text with RGB color (cyan text)
    const char *msg = "Hello, ttykit!";
    int len = strlen(msg);
    buffer_set_str_styled(buf, rows / 2, (cols - len) / 2, msg,
                          COLOR_RGB_INIT(0, 255, 255),  // cyan foreground
                          COLOR_DEFAULT_INIT,
                          ATTR_BOLD);

    // Draw border with 256-color (green)
    Color green = COLOR_INDEX(2);  // ANSI green
    for (int c = 0; c < cols; c++) {
        buffer_set_cell_styled(buf, 0, c, '-', green, COLOR_DEFAULT_INIT, ATTR_NONE);
        buffer_set_cell_styled(buf, rows - 1, c, '-', green, COLOR_DEFAULT_INIT, ATTR_NONE);
    }
    for (int r = 0; r < rows; r++) {
        buffer_set_cell_styled(buf, r, 0, '|', green, COLOR_DEFAULT_INIT, ATTR_NONE);
        buffer_set_cell_styled(buf, r, cols - 1, '|', green, COLOR_DEFAULT_INIT, ATTR_NONE);
    }

    // Demo different attributes
    int demo_row = rows / 2 + 2;
    buffer_set_str_styled(buf, demo_row, 2, "Bold", COLOR_INDEX(1), COLOR_DEFAULT_INIT, ATTR_BOLD);
    buffer_set_str_styled(buf, demo_row, 8, "Italic", COLOR_INDEX(3), COLOR_DEFAULT_INIT, ATTR_ITALIC);
    buffer_set_str_styled(buf, demo_row, 16, "Underline", COLOR_INDEX(4), COLOR_DEFAULT_INIT, ATTR_UNDERLINE);
    buffer_set_str_styled(buf, demo_row, 27, "Reverse", COLOR_INDEX(5), COLOR_DEFAULT_INIT, ATTR_REVERSE);

    // Show info at bottom (yellow on default background)
    char info[64];
    snprintf(info, sizeof(info), " %dx%d - Press 'q' to quit ", cols, rows);
    buffer_set_str_styled(buf, rows - 1, 2, info,
                          COLOR_INDEX(11),  // bright yellow
                          COLOR_DEFAULT_INIT,
                          ATTR_NONE);

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
