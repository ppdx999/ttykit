#include "buffer.h"
#include "event.h"
#include "layout.h"
#include "ttykit.h"
#include "widget.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_LINES 100
#define MAX_LINE_LEN 256

typedef enum { TAB_STATUS, TAB_LOG, TAB_BRANCHES, TAB_COUNT } Tab;

typedef struct {
  char lines[MAX_LINES][MAX_LINE_LEN];
  size_t count;
} LineBuffer;

typedef struct {
  Tab current_tab;
  LineBuffer status;
  LineBuffer log;
  LineBuffer branches;
  size_t selected[TAB_COUNT];
  char status_msg[128];
} AppState;

// Run command and capture output
static void run_command(const char *cmd, LineBuffer *buf) {
  buf->count = 0;
  FILE *fp = popen(cmd, "r");
  if (!fp)
    return;

  while (buf->count < MAX_LINES &&
         fgets(buf->lines[buf->count], MAX_LINE_LEN, fp)) {
    // Remove trailing newline
    size_t len = strlen(buf->lines[buf->count]);
    if (len > 0 && buf->lines[buf->count][len - 1] == '\n') {
      buf->lines[buf->count][len - 1] = '\0';
    }
    buf->count++;
  }
  pclose(fp);
}

// Refresh git data
static void refresh_data(AppState *s) {
  run_command("git status --short 2>/dev/null", &s->status);
  run_command("git log --oneline -20 2>/dev/null", &s->log);
  run_command("git branch 2>/dev/null", &s->branches);

  if (s->status.count == 0) {
    strcpy(s->status.lines[0], "No changes");
    s->status.count = 1;
  }
  if (s->log.count == 0) {
    strcpy(s->log.lines[0], "No commits");
    s->log.count = 1;
  }
  if (s->branches.count == 0) {
    strcpy(s->branches.lines[0], "No branches");
    s->branches.count = 1;
  }

  snprintf(s->status_msg, sizeof(s->status_msg),
           "1/2/3:tabs j/k:move r:refresh q:quit");
}

// Tab labels
static const char *g_tab_labels[] = {"Status", "Log", "Branches"};

// Build list data for current tab
static const char *g_list_items[MAX_LINES];

static void build_list_data(AppState *s) {
  LineBuffer *buf = NULL;
  switch (s->current_tab) {
  case TAB_STATUS:
    buf = &s->status;
    break;
  case TAB_LOG:
    buf = &s->log;
    break;
  case TAB_BRANCHES:
    buf = &s->branches;
    break;
  default:
    return;
  }

  for (size_t i = 0; i < buf->count; i++) {
    g_list_items[i] = buf->lines[i];
  }
}

// Get current line count
static size_t get_current_count(AppState *s) {
  switch (s->current_tab) {
  case TAB_STATUS:
    return s->status.count;
  case TAB_LOG:
    return s->log.count;
  case TAB_BRANCHES:
    return s->branches.count;
  default:
    return 0;
  }
}

// Declarative view function
Widget *view(AppState *s) {
  build_list_data(s);
  size_t count = get_current_count(s);
  size_t selected = s->selected[s->current_tab];

  const char *title = "";
  switch (s->current_tab) {
  case TAB_STATUS:
    title = "Status";
    break;
  case TAB_LOG:
    title = "Log";
    break;
  case TAB_BRANCHES:
    title = "Branches";
    break;
  default:
    break;
  }

  return VBOX(FILL, TABS(LEN(1), g_tab_labels, TAB_COUNT, s->current_tab),
              BLOCK(FILL, title, LIST(FILL, g_list_items, count, selected)),
              TEXT(LEN(1), s->status_msg));
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
  AppState state = {0};
  refresh_data(&state);

  int running = 1;
  Event event;

  // Initial render
  ui_frame_begin();
  widget_render(view(&state), buf, rect_from_size(cols, rows));
  ui_frame_end();
  buffer_render(buf);

  while (running && event_poll(&event, -1) >= 0) {
    int needs_redraw = 0;

    switch (event.type) {
    case EVENT_KEY:
      if (event.key.code == KEY_CHAR) {
        switch (event.key.ch) {
        case 'q':
          running = 0;
          break;
        case '1':
          state.current_tab = TAB_STATUS;
          needs_redraw = 1;
          break;
        case '2':
          state.current_tab = TAB_LOG;
          needs_redraw = 1;
          break;
        case '3':
          state.current_tab = TAB_BRANCHES;
          needs_redraw = 1;
          break;
        case 'j': {
          size_t count = get_current_count(&state);
          if (state.selected[state.current_tab] < count - 1) {
            state.selected[state.current_tab]++;
            needs_redraw = 1;
          }
          break;
        }
        case 'k':
          if (state.selected[state.current_tab] > 0) {
            state.selected[state.current_tab]--;
            needs_redraw = 1;
          }
          break;
        case 'r':
          refresh_data(&state);
          needs_redraw = 1;
          break;
        }
      } else if (event.key.code == KEY_TAB) {
        state.current_tab = (state.current_tab + 1) % TAB_COUNT;
        needs_redraw = 1;
      } else if (event.key.code == KEY_ESCAPE) {
        running = 0;
      } else if (event.key.code == KEY_DOWN) {
        size_t count = get_current_count(&state);
        if (state.selected[state.current_tab] < count - 1) {
          state.selected[state.current_tab]++;
          needs_redraw = 1;
        }
      } else if (event.key.code == KEY_UP) {
        if (state.selected[state.current_tab] > 0) {
          state.selected[state.current_tab]--;
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
  return 0;
}
