#include <stdio.h>
#include <string.h>
#include "ttykit.h"
#include "buffer.h"
#include "event.h"

static void draw_screen(Buffer *buf, int rows, int cols, const char *status) {
    buffer_clear(buf);

    // Draw centered text with RGB color (cyan text)
    const char *msg = "Hello, ttykit!";
    int len = strlen(msg);
    buffer_set_str_styled(buf, rows / 2, (cols - len) / 2, msg,
                          COLOR_RGB_INIT(0, 255, 255),
                          COLOR_DEFAULT_INIT,
                          ATTR_BOLD);

    // Draw border with 256-color (green)
    Color green = COLOR_INDEX(2);
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

    // Show status at bottom
    buffer_set_str_styled(buf, rows - 1, 2, status,
                          COLOR_INDEX(11),
                          COLOR_DEFAULT_INIT,
                          ATTR_NONE);

    buffer_render(buf);
}

int main(void) {
    if (tty_enable_raw_mode() == -1) {
        perror("tty_enable_raw_mode");
        return 1;
    }

    if (event_init() == -1) {
        tty_disable_raw_mode();
        return 1;
    }

    tty_enter_alternate_screen();
    tty_cursor_hide();

    int rows, cols;
    tty_get_size(&rows, &cols);

    Buffer *buf = buffer_create(rows, cols);
    if (!buf) {
        tty_cursor_show();
        tty_leave_alternate_screen();
        event_cleanup();
        tty_disable_raw_mode();
        return 1;
    }

    char status[64];
    snprintf(status, sizeof(status), " %dx%d | Press q to quit ", cols, rows);
    draw_screen(buf, rows, cols, status);

    int running = 1;
    Event event;

    while (running && event_poll(&event, -1) >= 0) {
        switch (event.type) {
            case EVENT_KEY:
                if (event.key.code == KEY_CHAR && event.key.ch == 'q') {
                    running = 0;
                } else if (event.key.code == KEY_CHAR) {
                    if (event.key.mod & MOD_CTRL) {
                        snprintf(status, sizeof(status), " Key: Ctrl+%c ", event.key.ch);
                    } else if (event.key.mod & MOD_ALT) {
                        snprintf(status, sizeof(status), " Key: Alt+%c ", event.key.ch);
                    } else {
                        snprintf(status, sizeof(status), " Key: '%c' ", event.key.ch);
                    }
                } else {
                    const char *name = "?";
                    switch (event.key.code) {
                        case KEY_UP: name = "Up"; break;
                        case KEY_DOWN: name = "Down"; break;
                        case KEY_LEFT: name = "Left"; break;
                        case KEY_RIGHT: name = "Right"; break;
                        case KEY_HOME: name = "Home"; break;
                        case KEY_END: name = "End"; break;
                        case KEY_PAGE_UP: name = "PageUp"; break;
                        case KEY_PAGE_DOWN: name = "PageDown"; break;
                        case KEY_ENTER: name = "Enter"; break;
                        case KEY_TAB: name = "Tab"; break;
                        case KEY_BACKSPACE: name = "Backspace"; break;
                        case KEY_ESCAPE: name = "Escape"; break;
                        case KEY_DELETE: name = "Delete"; break;
                        case KEY_INSERT: name = "Insert"; break;
                        case KEY_F1: name = "F1"; break;
                        case KEY_F2: name = "F2"; break;
                        case KEY_F3: name = "F3"; break;
                        case KEY_F4: name = "F4"; break;
                        case KEY_F5: name = "F5"; break;
                        case KEY_F6: name = "F6"; break;
                        case KEY_F7: name = "F7"; break;
                        case KEY_F8: name = "F8"; break;
                        case KEY_F9: name = "F9"; break;
                        case KEY_F10: name = "F10"; break;
                        case KEY_F11: name = "F11"; break;
                        case KEY_F12: name = "F12"; break;
                        default: break;
                    }
                    snprintf(status, sizeof(status), " Key: %s ", name);
                }
                draw_screen(buf, rows, cols, status);
                break;

            case EVENT_RESIZE:
                rows = event.resize.rows;
                cols = event.resize.cols;
                buffer_destroy(buf);
                buf = buffer_create(rows, cols);
                if (!buf) {
                    running = 0;
                    break;
                }
                snprintf(status, sizeof(status), " Resized: %dx%d ", cols, rows);
                draw_screen(buf, rows, cols, status);
                break;

            case EVENT_NONE:
                break;
        }
    }

    buffer_destroy(buf);
    tty_cursor_show();
    tty_leave_alternate_screen();
    event_cleanup();
    tty_disable_raw_mode();
    return 0;
}
