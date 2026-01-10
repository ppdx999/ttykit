#include "buffer.h"
#include "event.h"
#include "layout.h"
#include "ttykit.h"
#include "widget.h"
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#define MAX_ENTRIES 256
#define MAX_PATH 1024
#define MAX_PREVIEW_SIZE (32 * 1024) // 32KB for preview text

typedef struct {
  char name[256];
  int is_dir;
} Entry;

typedef struct {
  char cwd[MAX_PATH];
  Entry entries[MAX_ENTRIES];
  size_t entry_count;
  size_t selected;
  char preview[MAX_PREVIEW_SIZE];
  size_t preview_scroll;  // Scroll offset in lines
  char status[128];
} AppState;

// Read directory contents
static void read_directory(AppState *s) {
  DIR *dir = opendir(s->cwd);
  if (!dir) {
    s->entry_count = 0;
    snprintf(s->status, sizeof(s->status), "Cannot open directory");
    return;
  }

  s->entry_count = 0;
  struct dirent *ent;

  // Add parent directory first (except for root)
  if (strcmp(s->cwd, "/") != 0) {
    strcpy(s->entries[s->entry_count].name, "..");
    s->entries[s->entry_count].is_dir = 1;
    s->entry_count++;
  }

  while ((ent = readdir(dir)) != NULL && s->entry_count < MAX_ENTRIES) {
    // Skip . and ..
    if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0) {
      continue;
    }

    strncpy(s->entries[s->entry_count].name, ent->d_name, 255);
    s->entries[s->entry_count].name[255] = '\0';

    // Check if directory
    char fullpath[MAX_PATH];
    snprintf(fullpath, sizeof(fullpath), "%s/%s", s->cwd, ent->d_name);
    struct stat st;
    if (stat(fullpath, &st) == 0) {
      s->entries[s->entry_count].is_dir = S_ISDIR(st.st_mode);
    } else {
      s->entries[s->entry_count].is_dir = 0;
    }

    s->entry_count++;
  }

  closedir(dir);
  s->selected = 0;
  snprintf(s->status, sizeof(s->status),
           "%zu items | j/k:move h:up l:enter q:quit", s->entry_count);
}

// Read file preview
static void read_preview(AppState *s) {
  s->preview[0] = '\0';
  s->preview_scroll = 0;

  if (s->entry_count == 0 || s->selected >= s->entry_count) {
    return;
  }

  Entry *e = &s->entries[s->selected];
  if (e->is_dir) {
    // Show directory contents
    char dirpath[MAX_PATH];
    if (strcmp(e->name, "..") == 0) {
      // Parent directory
      strcpy(s->preview, "[Parent Directory]");
      return;
    }
    snprintf(dirpath, sizeof(dirpath), "%s/%s", s->cwd, e->name);

    DIR *dir = opendir(dirpath);
    if (!dir) {
      strcpy(s->preview, "[Cannot open directory]");
      return;
    }

    size_t offset = 0;
    struct dirent *ent;
    while ((ent = readdir(dir)) != NULL && offset < MAX_PREVIEW_SIZE - 256) {
      if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0) {
        continue;
      }
      size_t len = strlen(ent->d_name);
      memcpy(s->preview + offset, ent->d_name, len);
      offset += len;
      s->preview[offset++] = '\n';
    }
    if (offset > 0) {
      s->preview[offset - 1] = '\0'; // Remove last newline
    } else {
      strcpy(s->preview, "[Empty directory]");
    }
    closedir(dir);
    return;
  }

  // Regular file
  char fullpath[MAX_PATH];
  snprintf(fullpath, sizeof(fullpath), "%s/%s", s->cwd, e->name);

  FILE *f = fopen(fullpath, "r");
  if (!f) {
    strcpy(s->preview, "[Cannot read file]");
    return;
  }

  size_t offset = 0;
  char line[512];
  while (fgets(line, sizeof(line), f) && offset < MAX_PREVIEW_SIZE - 1) {
    size_t len = strlen(line);
    if (offset + len >= MAX_PREVIEW_SIZE - 1) {
      break;
    }
    memcpy(s->preview + offset, line, len);
    offset += len;
  }
  s->preview[offset] = '\0';

  fclose(f);

  if (offset == 0) {
    strcpy(s->preview, "[Empty file]");
  }
}

// Navigate to parent directory
static void go_parent(AppState *s) {
  if (strcmp(s->cwd, "/") == 0)
    return;

  char *last_slash = strrchr(s->cwd, '/');
  if (last_slash == s->cwd) {
    // Root
    s->cwd[1] = '\0';
  } else if (last_slash) {
    *last_slash = '\0';
  }

  read_directory(s);
  read_preview(s);
}

// Enter directory
static void enter_dir(AppState *s) {
  if (s->entry_count == 0 || s->selected >= s->entry_count)
    return;

  Entry *e = &s->entries[s->selected];
  if (!e->is_dir)
    return;

  if (strcmp(e->name, "..") == 0) {
    go_parent(s);
    return;
  }

  if (strcmp(s->cwd, "/") == 0) {
    snprintf(s->cwd, sizeof(s->cwd), "/%s", e->name);
  } else {
    size_t len = strlen(s->cwd);
    snprintf(s->cwd + len, sizeof(s->cwd) - len, "/%s", e->name);
  }

  read_directory(s);
  read_preview(s);
}

// Build entry names array for LIST widget
static const char *g_entry_names[MAX_ENTRIES];

static void build_entry_names(AppState *s) {
  for (size_t i = 0; i < s->entry_count; i++) {
    g_entry_names[i] = s->entries[i].name;
  }
}

// Get pointer to n-th line in preview (0-indexed)
static const char *get_preview_line(AppState *s, size_t line) {
  const char *p = s->preview;
  for (size_t i = 0; i < line && *p; i++) {
    while (*p && *p != '\n') p++;
    if (*p == '\n') p++;
  }
  return p;
}

// Count total lines in preview
static size_t count_preview_lines(AppState *s) {
  size_t count = 0;
  for (const char *p = s->preview; *p; p++) {
    if (*p == '\n') count++;
  }
  if (s->preview[0] != '\0') count++;  // Last line without newline
  return count;
}

// Declarative view function
Widget *view(AppState *s) {
  build_entry_names(s);

  // Get scrolled preview text
  const char *preview_text = get_preview_line(s, s->preview_scroll);

  return VBOX(
      FILL,
      BLOCK(FILL, s->cwd,
            HBOX(FILL,
                 LIST(PCT(30), g_entry_names, s->entry_count, s->selected),
                 VLINE(LEN(1)),
                 TEXT(FILL, preview_text))),
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
  AppState state = {0};
  if (getcwd(state.cwd, sizeof(state.cwd)) == NULL) {
    strcpy(state.cwd, "/");
  }
  read_directory(&state);
  read_preview(&state);

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
        // Ctrl+D: scroll preview down half page
        if ((event.key.mod & MOD_CTRL) && event.key.ch == 'd') {
          size_t total = count_preview_lines(&state);
          size_t half_page = (rows - 4) / 2;  // Approximate visible lines
          if (state.preview_scroll + half_page < total) {
            state.preview_scroll += half_page;
          } else if (total > 0) {
            state.preview_scroll = total - 1;
          }
          needs_redraw = 1;
          break;
        }
        // Ctrl+U: scroll preview up half page
        if ((event.key.mod & MOD_CTRL) && event.key.ch == 'u') {
          size_t half_page = (rows - 4) / 2;
          if (state.preview_scroll > half_page) {
            state.preview_scroll -= half_page;
          } else {
            state.preview_scroll = 0;
          }
          needs_redraw = 1;
          break;
        }
        switch (event.key.ch) {
        case 'q':
          running = 0;
          break;
        case 'j': // Down
          if (state.selected < state.entry_count - 1) {
            state.selected++;
            read_preview(&state);
            needs_redraw = 1;
          }
          break;
        case 'k': // Up
          if (state.selected > 0) {
            state.selected--;
            read_preview(&state);
            needs_redraw = 1;
          }
          break;
        case 'h': // Parent directory
          go_parent(&state);
          needs_redraw = 1;
          break;
        case 'l': // Enter directory
          enter_dir(&state);
          needs_redraw = 1;
          break;
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
