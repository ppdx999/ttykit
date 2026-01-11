#include "buffer.h"
#include "event.h"
#include "layout.h"
#include "ttykit.h"
#include "widget.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#define HISTORY_SIZE 60
#define MAX_PROCS 10

typedef struct {
  double cpu_usage;
  double mem_usage;
  double cpu_history[HISTORY_SIZE];
  double mem_history[HISTORY_SIZE];
  size_t history_count;
  char status[128];
} AppState;

// Simulate CPU/memory usage (random walk)
static void update_metrics(AppState *s) {
  // Random walk for CPU
  double delta = ((double)rand() / RAND_MAX - 0.5) * 0.1;
  s->cpu_usage += delta;
  if (s->cpu_usage < 0.05)
    s->cpu_usage = 0.05;
  if (s->cpu_usage > 0.95)
    s->cpu_usage = 0.95;

  // Random walk for memory
  delta = ((double)rand() / RAND_MAX - 0.5) * 0.05;
  s->mem_usage += delta;
  if (s->mem_usage < 0.2)
    s->mem_usage = 0.2;
  if (s->mem_usage > 0.9)
    s->mem_usage = 0.9;

  // Shift history and add new values
  if (s->history_count < HISTORY_SIZE) {
    s->cpu_history[s->history_count] = s->cpu_usage;
    s->mem_history[s->history_count] = s->mem_usage;
    s->history_count++;
  } else {
    for (size_t i = 0; i < HISTORY_SIZE - 1; i++) {
      s->cpu_history[i] = s->cpu_history[i + 1];
      s->mem_history[i] = s->mem_history[i + 1];
    }
    s->cpu_history[HISTORY_SIZE - 1] = s->cpu_usage;
    s->mem_history[HISTORY_SIZE - 1] = s->mem_usage;
  }

  snprintf(s->status, sizeof(s->status), "CPU: %.1f%% | MEM: %.1f%% | q:quit",
           s->cpu_usage * 100, s->mem_usage * 100);
}

// Process table data
static const char *g_proc_headers[] = {"PID", "NAME", "CPU%", "MEM%"};
static const char *g_proc_rows[MAX_PROCS][4];
static const char **g_proc_row_ptrs[MAX_PROCS];
static char g_proc_data[MAX_PROCS][4][32];

static void init_proc_table(void) {
  // Fake process data
  const char *names[] = {"init",   "systemd", "bash",  "vim",    "htop",
                         "chrome", "firefox", "slack", "docker", "node"};
  for (int i = 0; i < MAX_PROCS; i++) {
    snprintf(g_proc_data[i][0], 32, "%d", 1000 + i * 100);
    snprintf(g_proc_data[i][1], 32, "%s", names[i]);
    snprintf(g_proc_data[i][2], 32, "%.1f", (double)(rand() % 100) / 10.0);
    snprintf(g_proc_data[i][3], 32, "%.1f", (double)(rand() % 50) / 10.0);
    for (int j = 0; j < 4; j++) {
      g_proc_rows[i][j] = g_proc_data[i][j];
    }
    g_proc_row_ptrs[i] = g_proc_rows[i];
  }
}

static void update_proc_table(void) {
  // Update CPU/MEM values randomly
  for (int i = 0; i < MAX_PROCS; i++) {
    snprintf(g_proc_data[i][2], 32, "%.1f", (double)(rand() % 100) / 10.0);
    snprintf(g_proc_data[i][3], 32, "%.1f", (double)(rand() % 50) / 10.0);
  }
}

// Column widths for table
static const uint16_t g_col_widths[] = {8, 12, 8, 8};

// Declarative view function
Widget *view(AppState *s) {
  return VBOX(
      FILL,
      // CPU section
      BLOCK(LEN(5), "CPU",
            VBOX(FILL, GAUGE(LEN(1), s->cpu_usage, NULL, COLOR_INDEX(10)),
                 SPARKLINE(FILL, s->cpu_history, s->history_count,
                           COLOR_INDEX(10)))),
      // Memory section
      BLOCK(LEN(5), "Memory",
            VBOX(FILL, GAUGE(LEN(1), s->mem_usage, NULL, COLOR_INDEX(12)),
                 SPARKLINE(FILL, s->mem_history, s->history_count,
                           COLOR_INDEX(12)))),
      // Process table
      BLOCK(FILL, "Processes",
            TABLE(FILL, g_proc_headers, (const char ***)g_proc_row_ptrs, 4,
                  MAX_PROCS, g_col_widths)),
      // Status bar
      TEXT(LEN(1), s->status));
}

int main(void) {
  srand(time(NULL));

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
  state.cpu_usage = 0.3;
  state.mem_usage = 0.5;
  init_proc_table();
  update_metrics(&state);

  int running = 1;
  Event event;

  // Initial render
  ui_frame_begin();
  widget_render(view(&state), buf, rect_from_size(cols, rows));
  ui_frame_end();
  buffer_render(buf);

  while (running) {
    // Poll with 500ms timeout for auto-refresh
    int ret = event_poll(&event, 500);

    if (ret < 0) {
      break;
    }

    if (ret > 0) {
      switch (event.type) {
      case EVENT_KEY:
        if (event.key.code == KEY_CHAR && event.key.ch == 'q') {
          running = 0;
        } else if (event.key.code == KEY_ESCAPE) {
          running = 0;
        }
        break;

      case EVENT_RESIZE:
        rows = event.resize.rows;
        cols = event.resize.cols;
        buffer_destroy(buf);
        buf = buffer_create(rows, cols);
        if (!buf) {
          running = 0;
        }
        break;

      case EVENT_NONE:
        break;
      }
    }

    // Update metrics periodically
    update_metrics(&state);
    update_proc_table();

    // Redraw
    buffer_clear(buf);
    ui_frame_begin();
    widget_render(view(&state), buf, rect_from_size(cols, rows));
    ui_frame_end();
    buffer_render(buf);
  }

  buffer_destroy(buf);
  tty_cursor_show();
  tty_leave_alternate_screen();
  event_cleanup();
  tty_disable_raw_mode();
  return 0;
}
