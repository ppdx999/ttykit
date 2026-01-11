#include "buffer.h"
#include "event.h"
#include "layout.h"
#include "ttykit.h"
#include "widget.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_ENTRIES 4096
#define MAX_LINE 1024
#define MAX_QUERY 256

typedef struct {
  char **lines;
  size_t count;
  size_t capacity;
} Lines;

typedef struct {
  Lines all;
  const char *filtered[MAX_ENTRIES];
  size_t filtered_count;
  size_t selected;
  char query[MAX_QUERY];
  size_t cursor;
  char status[128];
} AppState;

// Read lines from stdin
static void read_stdin(Lines *lines) {
  lines->capacity = 256;
  lines->count = 0;
  lines->lines = malloc(sizeof(char *) * lines->capacity);

  char buf[MAX_LINE];
  while (fgets(buf, sizeof(buf), stdin)) {
    // Remove trailing newline
    size_t len = strlen(buf);
    if (len > 0 && buf[len - 1] == '\n') {
      buf[len - 1] = '\0';
      len--;
    }

    if (lines->count >= lines->capacity) {
      lines->capacity *= 2;
      lines->lines = realloc(lines->lines, sizeof(char *) * lines->capacity);
    }

    lines->lines[lines->count] = strdup(buf);
    lines->count++;
  }
}

static void free_lines(Lines *lines) {
  for (size_t i = 0; i < lines->count; i++) {
    free(lines->lines[i]);
  }
  free(lines->lines);
}

// Simple substring match (case-insensitive)
static int matches(const char *str, const char *query) {
  if (query[0] == '\0')
    return 1;

  size_t str_len = strlen(str);
  size_t query_len = strlen(query);

  for (size_t i = 0; i + query_len <= str_len; i++) {
    int match = 1;
    for (size_t j = 0; j < query_len; j++) {
      char sc = str[i + j];
      char qc = query[j];
      if (sc >= 'A' && sc <= 'Z')
        sc += 32;
      if (qc >= 'A' && qc <= 'Z')
        qc += 32;
      if (sc != qc) {
        match = 0;
        break;
      }
    }
    if (match)
      return 1;
  }
  return 0;
}

// Filter entries based on query
static void filter_entries(AppState *s) {
  s->filtered_count = 0;
  for (size_t i = 0; i < s->all.count && s->filtered_count < MAX_ENTRIES; i++) {
    if (matches(s->all.lines[i], s->query)) {
      s->filtered[s->filtered_count] = s->all.lines[i];
      s->filtered_count++;
    }
  }
  s->selected = 0;
  snprintf(s->status, sizeof(s->status), "%zu/%zu", s->filtered_count,
           s->all.count);
}

// Insert character at cursor position
static void insert_char(AppState *s, char ch) {
  size_t len = strlen(s->query);
  if (len >= MAX_QUERY - 1)
    return;

  for (size_t i = len + 1; i > s->cursor; i--) {
    s->query[i] = s->query[i - 1];
  }
  s->query[s->cursor] = ch;
  s->cursor++;
}

// Delete character before cursor
static void delete_char(AppState *s) {
  if (s->cursor == 0)
    return;

  size_t len = strlen(s->query);
  for (size_t i = s->cursor - 1; i < len; i++) {
    s->query[i] = s->query[i + 1];
  }
  s->cursor--;
}

// Declarative view function
Widget *view(AppState *s) {
  return VBOX(FILL, INPUT(LEN(1), s->query, s->cursor, "> "), HLINE(LEN(1)),
              LIST(FILL, s->filtered, s->filtered_count, s->selected),
              HLINE(LEN(1)), TEXT(LEN(1), s->status));
}

int main(void) {
  // Read stdin before entering raw mode
  Lines lines = {0};
  read_stdin(&lines);

  if (tty_enable_raw_mode() == -1) {
    perror("tty_enable_raw_mode");
    free_lines(&lines);
    return 1;
  }

  if (event_init() == -1) {
    tty_disable_raw_mode();
    free_lines(&lines);
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
    free_lines(&lines);
    return 1;
  }

  // Initialize state
  AppState state = {0};
  state.all = lines;
  filter_entries(&state);

  int running = 1;
  Event event;
  const char *selected = NULL;

  // Initial render
  ui_frame_begin();
  widget_render(view(&state), buf, rect_from_size(cols, rows));
  ui_frame_end();
  buffer_render(buf);

  while (running && event_poll(&event, -1) >= 0) {
    int needs_redraw = 0;
    int needs_filter = 0;

    switch (event.type) {
    case EVENT_KEY:
      if (event.key.code == KEY_ESCAPE) {
        running = 0;
      } else if (event.key.code == KEY_ENTER) {
        if (state.filtered_count > 0) {
          selected = state.filtered[state.selected];
          running = 0;
        }
      } else if (event.key.code == KEY_BACKSPACE) {
        delete_char(&state);
        needs_filter = 1;
        needs_redraw = 1;
      } else if (event.key.code == KEY_UP ||
                 (event.key.code == KEY_CHAR && event.key.ch == 'p' &&
                  (event.key.mod & MOD_CTRL))) {
        if (state.selected > 0) {
          state.selected--;
          needs_redraw = 1;
        }
      } else if (event.key.code == KEY_DOWN ||
                 (event.key.code == KEY_CHAR && event.key.ch == 'n' &&
                  (event.key.mod & MOD_CTRL))) {
        if (state.selected < state.filtered_count - 1) {
          state.selected++;
          needs_redraw = 1;
        }
      } else if (event.key.code == KEY_CHAR) {
        if (event.key.ch >= 32 && event.key.ch < 127) {
          insert_char(&state, event.key.ch);
          needs_filter = 1;
          needs_redraw = 1;
        }
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
      needs_redraw = 1;
      break;

    case EVENT_NONE:
      break;
    }

    if (needs_filter) {
      filter_entries(&state);
    }

    if (needs_redraw) {
      buffer_clear(buf);
      ui_frame_begin();
      widget_render(view(&state), buf, rect_from_size(cols, rows));
      ui_frame_end();
      buffer_render(buf);
    }
  }

  buffer_destroy(buf);
  tty_cursor_show();
  tty_leave_alternate_screen();
  event_cleanup();
  tty_disable_raw_mode();

  if (selected) {
    printf("%s\n", selected);
  }

  free_lines(&lines);
  return selected ? 0 : 1;
}
