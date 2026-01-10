#include <stdio.h>
#include <string.h>
#include "ttykit.h"
#include "buffer.h"
#include "event.h"
#include "layout.h"

// Draw a horizontal line in a rect
static void draw_hline(Buffer *buf, Rect r, char ch, Color fg) {
    for (uint16_t c = r.x; c < r.x + r.width; c++) {
        buffer_set_cell_styled(buf, r.y, c, ch, fg, COLOR_DEFAULT_INIT, ATTR_NONE);
    }
}

// Fill a rect with a character
static void fill_rect(Buffer *buf, Rect r, char ch, Color fg, Color bg) {
    for (uint16_t row = r.y; row < r.y + r.height; row++) {
        for (uint16_t col = r.x; col < r.x + r.width; col++) {
            buffer_set_cell_styled(buf, row, col, ch, fg, bg, ATTR_NONE);
        }
    }
}

// Draw centered text in a rect
static void draw_centered(Buffer *buf, Rect r, const char *text, Color fg, uint8_t attrs) {
    int len = strlen(text);
    int x = r.x + (r.width - len) / 2;
    int y = r.y + r.height / 2;
    if (x < r.x) x = r.x;
    buffer_set_str_styled(buf, y, x, text, fg, COLOR_DEFAULT_INIT, attrs);
}

static void draw_screen(Buffer *buf, int rows, int cols, const char *status) {
    buffer_clear(buf);

    Rect screen = rect_from_size(cols, rows);

    // Split: Header(1) + Main(fill) + Footer(1)
    Constraint v_constraints[] = {
        CONSTRAINT_LEN(1),      // Header
        CONSTRAINT_FILL_W(1),   // Main
        CONSTRAINT_LEN(1)       // Footer
    };
    Rect v_areas[3];
    layout_split(screen, DIRECTION_VERTICAL, v_constraints, 3, v_areas);

    Rect header = v_areas[0];
    Rect main_area = v_areas[1];
    Rect footer = v_areas[2];

    // Draw header line
    draw_hline(buf, header, '-', COLOR_INDEX(2));
    buffer_set_str_styled(buf, header.y, header.x + 1, " ttykit Layout Demo ",
                          COLOR_INDEX(11), COLOR_DEFAULT_INIT, ATTR_BOLD);

    // Split main area horizontally: Sidebar(20%) + Content(fill)
    Constraint h_constraints[] = {
        CONSTRAINT_PCT(20),     // Sidebar
        CONSTRAINT_FILL_W(1)    // Content
    };
    Rect h_areas[2];
    layout_split(main_area, DIRECTION_HORIZONTAL, h_constraints, 2, h_areas);

    Rect sidebar = h_areas[0];
    Rect content = h_areas[1];

    // Draw sidebar background
    fill_rect(buf, sidebar, ' ', COLOR_DEFAULT_INIT, COLOR_INDEX(236));

    // Sidebar labels
    buffer_set_str_styled(buf, sidebar.y, sidebar.x + 1, "Sidebar",
                          COLOR_INDEX(14), COLOR_INDEX(236), ATTR_BOLD);
    buffer_set_str_styled(buf, sidebar.y + 2, sidebar.x + 1, "> Item 1",
                          COLOR_INDEX(7), COLOR_INDEX(236), ATTR_NONE);
    buffer_set_str_styled(buf, sidebar.y + 3, sidebar.x + 1, "  Item 2",
                          COLOR_INDEX(8), COLOR_INDEX(236), ATTR_NONE);
    buffer_set_str_styled(buf, sidebar.y + 4, sidebar.x + 1, "  Item 3",
                          COLOR_INDEX(8), COLOR_INDEX(236), ATTR_NONE);

    // Draw content
    draw_centered(buf, content, "Hello, ttykit!",
                  COLOR_RGB_INIT(0, 255, 255), ATTR_BOLD);

    // Demo attributes in content area
    int demo_row = content.y + content.height / 2 + 2;
    int demo_col = content.x + 2;
    buffer_set_str_styled(buf, demo_row, demo_col, "Bold", COLOR_INDEX(1), COLOR_DEFAULT_INIT, ATTR_BOLD);
    buffer_set_str_styled(buf, demo_row, demo_col + 6, "Italic", COLOR_INDEX(3), COLOR_DEFAULT_INIT, ATTR_ITALIC);
    buffer_set_str_styled(buf, demo_row, demo_col + 14, "Underline", COLOR_INDEX(4), COLOR_DEFAULT_INIT, ATTR_UNDERLINE);
    buffer_set_str_styled(buf, demo_row, demo_col + 25, "Reverse", COLOR_INDEX(5), COLOR_DEFAULT_INIT, ATTR_REVERSE);

    // Draw footer line
    draw_hline(buf, footer, '-', COLOR_INDEX(2));
    buffer_set_str_styled(buf, footer.y, footer.x + 1, status,
                          COLOR_INDEX(11), COLOR_DEFAULT_INIT, ATTR_NONE);

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
