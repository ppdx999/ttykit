#include "buffer.h"
#include "ttykit.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

Buffer *buffer_create(int rows, int cols) {
  Buffer *buf = malloc(sizeof(Buffer));
  if (!buf)
    return NULL;

  buf->rows = rows;
  buf->cols = cols;
  buf->cells = malloc(sizeof(Cell) * rows * cols);
  if (!buf->cells) {
    free(buf);
    return NULL;
  }

  buffer_clear(buf);
  return buf;
}

void buffer_destroy(Buffer *buf) {
  if (buf) {
    free(buf->cells);
    free(buf);
  }
}

void buffer_clear(Buffer *buf) {
  for (int i = 0; i < buf->rows * buf->cols; i++) {
    buf->cells[i].ch = ' ';
    buf->cells[i].fg = COLOR_DEFAULT_INIT;
    buf->cells[i].bg = COLOR_DEFAULT_INIT;
    buf->cells[i].attrs = ATTR_NONE;
  }
}

Cell *buffer_get_cell(Buffer *buf, int row, int col) {
  if (row < 0 || row >= buf->rows || col < 0 || col >= buf->cols) {
    return NULL;
  }
  return &buf->cells[row * buf->cols + col];
}

void buffer_set_cell(Buffer *buf, int row, int col, char ch) {
  Cell *cell = buffer_get_cell(buf, row, col);
  if (cell) {
    cell->ch = ch;
    cell->fg = COLOR_DEFAULT_INIT;
    cell->bg = COLOR_DEFAULT_INIT;
    cell->attrs = ATTR_NONE;
  }
}

void buffer_set_cell_styled(Buffer *buf, int row, int col, char ch, Color fg,
                            Color bg, uint8_t attrs) {
  Cell *cell = buffer_get_cell(buf, row, col);
  if (cell) {
    cell->ch = ch;
    cell->fg = fg;
    cell->bg = bg;
    cell->attrs = attrs;
  }
}

#define TAB_WIDTH 4

void buffer_set_str(Buffer *buf, int row, int col, const char *str) {
  int start_col = col;
  while (*str && col < buf->cols) {
    if (*str == '\t') {
      int spaces = TAB_WIDTH - ((col - start_col) % TAB_WIDTH);
      for (int i = 0; i < spaces && col < buf->cols; i++) {
        buffer_set_cell(buf, row, col, ' ');
        col++;
      }
    } else {
      buffer_set_cell(buf, row, col, *str);
      col++;
    }
    str++;
  }
}

void buffer_set_str_styled(Buffer *buf, int row, int col, const char *str,
                           Color fg, Color bg, uint8_t attrs) {
  int start_col = col;
  while (*str && col < buf->cols) {
    if (*str == '\t') {
      int spaces = TAB_WIDTH - ((col - start_col) % TAB_WIDTH);
      for (int i = 0; i < spaces && col < buf->cols; i++) {
        buffer_set_cell_styled(buf, row, col, ' ', fg, bg, attrs);
        col++;
      }
    } else {
      buffer_set_cell_styled(buf, row, col, *str, fg, bg, attrs);
      col++;
    }
    str++;
  }
}

// Helper to check if two colors are equal
static int color_eq(Color a, Color b) {
  if (a.type != b.type)
    return 0;
  switch (a.type) {
  case COLOR_DEFAULT:
    return 1;
  case COLOR_INDEXED:
    return a.r == b.r; // 'r' stores the index
  case COLOR_RGB:
    return a.r == b.r && a.g == b.g && a.b == b.b;
  }
  return 0;
}

// Write ANSI escape sequence for a color
// is_fg: 1 for foreground, 0 for background
static int write_color_escape(char *out, Color color, int is_fg) {
  int pos = 0;
  int base = is_fg ? 38 : 48;
  int default_code = is_fg ? 39 : 49;

  switch (color.type) {
  case COLOR_DEFAULT:
    pos += sprintf(out + pos, "\x1b[%dm", default_code);
    break;
  case COLOR_INDEXED:
    pos += sprintf(out + pos, "\x1b[%d;5;%dm", base,
                   color.r); // 'r' stores the index
    break;
  case COLOR_RGB:
    pos += sprintf(out + pos, "\x1b[%d;2;%d;%d;%dm", base, color.r, color.g,
                   color.b);
    break;
  }
  return pos;
}

// Write ANSI escape sequences for attributes
static int write_attr_escape(char *out, uint8_t attrs) {
  int pos = 0;
  if (attrs & ATTR_BOLD)
    pos += sprintf(out + pos, "\x1b[1m");
  if (attrs & ATTR_DIM)
    pos += sprintf(out + pos, "\x1b[2m");
  if (attrs & ATTR_ITALIC)
    pos += sprintf(out + pos, "\x1b[3m");
  if (attrs & ATTR_UNDERLINE)
    pos += sprintf(out + pos, "\x1b[4m");
  if (attrs & ATTR_BLINK)
    pos += sprintf(out + pos, "\x1b[5m");
  if (attrs & ATTR_REVERSE)
    pos += sprintf(out + pos, "\x1b[7m");
  if (attrs & ATTR_HIDDEN)
    pos += sprintf(out + pos, "\x1b[8m");
  if (attrs & ATTR_STRIKE)
    pos += sprintf(out + pos, "\x1b[9m");
  return pos;
}

void buffer_render(Buffer *buf) {
  tty_cursor_home();

  // Allocate generous buffer for output with escape sequences
  // Each cell could need ~30 bytes for full color+attr sequences
  size_t max_size = buf->rows * buf->cols * 32 + 64;
  char *out = malloc(max_size);
  if (!out)
    return;

  int pos = 0;
  Color cur_fg = COLOR_DEFAULT_INIT;
  Color cur_bg = COLOR_DEFAULT_INIT;
  uint8_t cur_attrs = ATTR_NONE;

  for (int r = 0; r < buf->rows; r++) {
    for (int c = 0; c < buf->cols; c++) {
      Cell *cell = &buf->cells[r * buf->cols + c];

      // Check if style changed
      int fg_changed = !color_eq(cell->fg, cur_fg);
      int bg_changed = !color_eq(cell->bg, cur_bg);
      int attrs_changed = (cell->attrs != cur_attrs);

      if (fg_changed || bg_changed || attrs_changed) {
        // Reset all attributes first, then apply new ones
        pos += sprintf(out + pos, "\x1b[0m");
        pos += write_color_escape(out + pos, cell->fg, 1);
        pos += write_color_escape(out + pos, cell->bg, 0);
        pos += write_attr_escape(out + pos, cell->attrs);

        cur_fg = cell->fg;
        cur_bg = cell->bg;
        cur_attrs = cell->attrs;
      }

      out[pos++] = cell->ch;
    }
  }

  // Reset attributes at the end
  pos += sprintf(out + pos, "\x1b[0m");

  write(STDOUT_FILENO, out, pos);
  free(out);
}
