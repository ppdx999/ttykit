#include "widget.h"
#include "buffer.h"
#include "event.h"
#include "layout.h"
#include "ttykit.h"
#include <stdio.h>
#include <string.h>

typedef struct {
  const char *files[10];
  size_t file_count;
  size_t selected;
  char status[64];
} AppState;

// Declarative view function
Widget *view(AppState *s) {
  return VBOX(
      FILL,
      BLOCK(
          FILL, "Files",
          HBOX(FILL, LIST(PCT(30), s->files, s->file_count, s->selected),
               BLOCK(FILL, "Preview", TEXT(FILL, "Select a file to preview")))),
      TEXT(LEN(1), s->status));
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

  // Initialize state
  AppState state = {
      .files = {"main.c", "utils.c", "config.h", "Makefile", "README.md", NULL},
      .file_count = 5,
      .selected = 0};
  snprintf(state.status, sizeof(state.status),
           " %dx%d | Up/Down: navigate, q: quit ", cols, rows);

  int running = 1;
  Event event;

  // Initial render
  ui_frame_begin();
  Widget *ui = view(&state);
  widget_render(ui, buf, rect_from_size(cols, rows));
  ui_frame_end();
  buffer_render(buf);

  while (running && event_poll(&event, -1) >= 0) {
    int needs_redraw = 0;

    switch (event.type) {
    case EVENT_KEY:
      if (event.key.code == KEY_CHAR && event.key.ch == 'q') {
        running = 0;
      } else if (event.key.code == KEY_UP) {
        if (state.selected > 0) {
          state.selected--;
          needs_redraw = 1;
        }
      } else if (event.key.code == KEY_DOWN) {
        if (state.selected < state.file_count - 1) {
          state.selected++;
          needs_redraw = 1;
        }
      } else if (event.key.code == KEY_CHAR) {
        snprintf(state.status, sizeof(state.status),
                 " Key: '%c' | Selected: %s ", event.key.ch,
                 state.files[state.selected]);
        needs_redraw = 1;
      }
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
      snprintf(state.status, sizeof(state.status), " Resized: %dx%d ", cols,
               rows);
      needs_redraw = 1;
      break;

    case EVENT_NONE:
      break;
    }

    if (needs_redraw) {
      buffer_clear(buf);
      ui_frame_begin();
      Widget *ui = view(&state);
      widget_render(ui, buf, rect_from_size(cols, rows));
      ui_frame_end();
      buffer_render(buf);
    }
  }

  buffer_destroy(buf);
  tty_cursor_show();
  tty_leave_alternate_screen();
  event_cleanup();
  tty_disable_raw_mode();
  return 0;
}
