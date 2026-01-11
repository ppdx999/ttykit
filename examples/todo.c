#include "buffer.h"
#include "event.h"
#include "layout.h"
#include "ttykit.h"
#include "widget.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_TASKS 100
#define MAX_TASK_LEN 256

typedef struct {
  char text[MAX_TASK_LEN];
  int completed;
} Task;

typedef struct {
  Task tasks[MAX_TASKS];
  size_t task_count;
  size_t selected;
  char input[MAX_TASK_LEN];
  size_t cursor;
  int input_mode; // 0 = navigation, 1 = input
  char status[128];
} AppState;

// Calculate progress
static double calc_progress(AppState *s) {
  if (s->task_count == 0)
    return 0.0;
  size_t completed = 0;
  for (size_t i = 0; i < s->task_count; i++) {
    if (s->tasks[i].completed)
      completed++;
  }
  return (double)completed / s->task_count;
}

// Update status
static void update_status(AppState *s) {
  size_t completed = 0;
  for (size_t i = 0; i < s->task_count; i++) {
    if (s->tasks[i].completed)
      completed++;
  }
  if (s->input_mode) {
    snprintf(s->status, sizeof(s->status),
             "Type task, Enter to add, Esc to cancel");
  } else {
    snprintf(s->status, sizeof(s->status),
             "%zu/%zu done | a:add x:toggle d:delete q:quit", completed,
             s->task_count);
  }
}

// Add task
static void add_task(AppState *s, const char *text) {
  if (s->task_count >= MAX_TASKS || text[0] == '\0')
    return;
  strncpy(s->tasks[s->task_count].text, text, MAX_TASK_LEN - 1);
  s->tasks[s->task_count].text[MAX_TASK_LEN - 1] = '\0';
  s->tasks[s->task_count].completed = 0;
  s->task_count++;
}

// Delete task
static void delete_task(AppState *s) {
  if (s->task_count == 0 || s->selected >= s->task_count)
    return;
  for (size_t i = s->selected; i < s->task_count - 1; i++) {
    s->tasks[i] = s->tasks[i + 1];
  }
  s->task_count--;
  if (s->selected > 0 && s->selected >= s->task_count) {
    s->selected--;
  }
}

// Toggle task
static void toggle_task(AppState *s) {
  if (s->task_count == 0 || s->selected >= s->task_count)
    return;
  s->tasks[s->selected].completed = !s->tasks[s->selected].completed;
}

// Insert character
static void insert_char(AppState *s, char ch) {
  size_t len = strlen(s->input);
  if (len >= MAX_TASK_LEN - 1)
    return;
  for (size_t i = len + 1; i > s->cursor; i--) {
    s->input[i] = s->input[i - 1];
  }
  s->input[s->cursor] = ch;
  s->cursor++;
}

// Delete character
static void delete_char(AppState *s) {
  if (s->cursor == 0)
    return;
  size_t len = strlen(s->input);
  for (size_t i = s->cursor - 1; i < len; i++) {
    s->input[i] = s->input[i + 1];
  }
  s->cursor--;
}

// Build checkbox data
static const char *g_task_labels[MAX_TASKS];
static int g_task_checked[MAX_TASKS];

static void build_checkbox_data(AppState *s) {
  for (size_t i = 0; i < s->task_count; i++) {
    g_task_labels[i] = s->tasks[i].text;
    g_task_checked[i] = s->tasks[i].completed;
  }
}

// Declarative view function
Widget *view(AppState *s) {
  build_checkbox_data(s);

  Widget *content;
  if (s->task_count == 0) {
    content = TEXT(FILL, "No tasks. Press 'a' to add one.");
  } else {
    content = CHECKBOX(FILL, g_task_labels, g_task_checked, s->task_count,
                       s->selected);
  }

  if (s->input_mode) {
    return VBOX(FILL,
                BLOCK(FILL, "Tasks", content),
                HLINE(LEN(1)),
                INPUT(LEN(1), s->input, s->cursor, "New: "),
                PROGRESS(LEN(1), calc_progress(s), "Progress ", 1),
                TEXT(LEN(1), s->status));
  } else {
    return VBOX(FILL,
                BLOCK(FILL, "Tasks", content),
                PROGRESS(LEN(1), calc_progress(s), "Progress ", 1),
                TEXT(LEN(1), s->status));
  }
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

  // Initialize state with sample tasks
  AppState state = {0};
  add_task(&state, "Learn ttykit widgets");
  add_task(&state, "Build a TUI application");
  add_task(&state, "Add more features");
  state.tasks[0].completed = 1;
  update_status(&state);

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
      if (state.input_mode) {
        // Input mode
        if (event.key.code == KEY_ESCAPE) {
          state.input_mode = 0;
          state.input[0] = '\0';
          state.cursor = 0;
          needs_redraw = 1;
        } else if (event.key.code == KEY_ENTER) {
          add_task(&state, state.input);
          state.input_mode = 0;
          state.input[0] = '\0';
          state.cursor = 0;
          needs_redraw = 1;
        } else if (event.key.code == KEY_BACKSPACE) {
          delete_char(&state);
          needs_redraw = 1;
        } else if (event.key.code == KEY_CHAR) {
          if (event.key.ch >= 32 && event.key.ch < 127) {
            insert_char(&state, event.key.ch);
            needs_redraw = 1;
          }
        }
      } else {
        // Navigation mode
        if (event.key.code == KEY_CHAR) {
          switch (event.key.ch) {
          case 'q':
            running = 0;
            break;
          case 'a':
            state.input_mode = 1;
            needs_redraw = 1;
            break;
          case 'x':
          case ' ':
            toggle_task(&state);
            needs_redraw = 1;
            break;
          case 'd':
            delete_task(&state);
            needs_redraw = 1;
            break;
          case 'j':
            if (state.selected < state.task_count - 1) {
              state.selected++;
              needs_redraw = 1;
            }
            break;
          case 'k':
            if (state.selected > 0) {
              state.selected--;
              needs_redraw = 1;
            }
            break;
          }
        } else if (event.key.code == KEY_DOWN) {
          if (state.selected < state.task_count - 1) {
            state.selected++;
            needs_redraw = 1;
          }
        } else if (event.key.code == KEY_UP) {
          if (state.selected > 0) {
            state.selected--;
            needs_redraw = 1;
          }
        } else if (event.key.code == KEY_ESCAPE) {
          running = 0;
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
      update_status(&state);
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
