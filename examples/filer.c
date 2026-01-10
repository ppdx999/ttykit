#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>
#include "ttykit.h"
#include "buffer.h"
#include "event.h"
#include "layout.h"
#include "widget.h"

#define MAX_ENTRIES 256
#define MAX_PATH 1024
#define MAX_PREVIEW_LINES 100
#define MAX_LINE_LEN 256

typedef struct {
    char name[256];
    int is_dir;
} Entry;

typedef struct {
    char cwd[MAX_PATH];
    Entry entries[MAX_ENTRIES];
    size_t entry_count;
    size_t selected;
    char preview[MAX_PREVIEW_LINES][MAX_LINE_LEN];
    size_t preview_lines;
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
    snprintf(s->status, sizeof(s->status), "%zu items | j/k:move h:up l:enter q:quit", s->entry_count);
}

// Read file preview
static void read_preview(AppState *s) {
    s->preview_lines = 0;

    if (s->entry_count == 0 || s->selected >= s->entry_count) {
        return;
    }

    Entry *e = &s->entries[s->selected];
    if (e->is_dir) {
        strcpy(s->preview[0], "[Directory]");
        s->preview_lines = 1;
        return;
    }

    char fullpath[MAX_PATH];
    snprintf(fullpath, sizeof(fullpath), "%s/%s", s->cwd, e->name);

    FILE *f = fopen(fullpath, "r");
    if (!f) {
        strcpy(s->preview[0], "[Cannot read file]");
        s->preview_lines = 1;
        return;
    }

    while (s->preview_lines < MAX_PREVIEW_LINES &&
           fgets(s->preview[s->preview_lines], MAX_LINE_LEN, f)) {
        // Remove newline
        size_t len = strlen(s->preview[s->preview_lines]);
        if (len > 0 && s->preview[s->preview_lines][len-1] == '\n') {
            s->preview[s->preview_lines][len-1] = '\0';
        }
        s->preview_lines++;
    }

    fclose(f);

    if (s->preview_lines == 0) {
        strcpy(s->preview[0], "[Empty file]");
        s->preview_lines = 1;
    }
}

// Navigate to parent directory
static void go_parent(AppState *s) {
    if (strcmp(s->cwd, "/") == 0) return;

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
    if (s->entry_count == 0 || s->selected >= s->entry_count) return;

    Entry *e = &s->entries[s->selected];
    if (!e->is_dir) return;

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

// Build preview lines array for LIST widget
static const char *g_preview_lines[MAX_PREVIEW_LINES];

static void build_preview_lines(AppState *s) {
    for (size_t i = 0; i < s->preview_lines; i++) {
        g_preview_lines[i] = s->preview[i];
    }
}

// Declarative view function
Widget *view(AppState *s) {
    build_entry_names(s);
    build_preview_lines(s);

    return VBOX(FILL,
        BLOCK(FILL, s->cwd,
            HBOX(FILL,
                LIST(PCT(30), g_entry_names, s->entry_count, s->selected),
                BLOCK(FILL, "Preview",
                    LIST(FILL, g_preview_lines, s->preview_lines, 0)
                )
            )
        ),
        TEXT(LEN(1), s->status)
    );
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
                    switch (event.key.ch) {
                        case 'q':
                            running = 0;
                            break;
                        case 'j':  // Down
                            if (state.selected < state.entry_count - 1) {
                                state.selected++;
                                read_preview(&state);
                                needs_redraw = 1;
                            }
                            break;
                        case 'k':  // Up
                            if (state.selected > 0) {
                                state.selected--;
                                read_preview(&state);
                                needs_redraw = 1;
                            }
                            break;
                        case 'h':  // Parent directory
                            go_parent(&state);
                            needs_redraw = 1;
                            break;
                        case 'l':  // Enter directory
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
