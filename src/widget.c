#include "widget.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Frame arena
#define ARENA_SIZE (64 * 1024) // 64KB per frame

typedef struct {
  uint8_t *buf;
  size_t size;
  size_t offset;
} Arena;

static Arena g_frame_arena;
static int g_arena_initialized = 0;

static void *arena_alloc(Arena *a, size_t size) {
  // Align to 8 bytes
  size = (size + 7) & ~7;
  if (a->offset + size > a->size) {
    return NULL; // Out of memory
  }
  void *ptr = a->buf + a->offset;
  a->offset += size;
  return ptr;
}

static void arena_reset(Arena *a) { a->offset = 0; }

void ui_frame_begin(void) {
  if (!g_arena_initialized) {
    g_frame_arena.buf = malloc(ARENA_SIZE);
    g_frame_arena.size = ARENA_SIZE;
    g_frame_arena.offset = 0;
    g_arena_initialized = 1;
  }
  arena_reset(&g_frame_arena);
}

void ui_frame_end(void) {
  // No-op for now, reserved for future use
}

// Widget constructors

Widget *widget_vbox(Constraint c, Widget **children) {
  Widget *w = arena_alloc(&g_frame_arena, sizeof(Widget));
  if (!w)
    return NULL;

  w->type = WIDGET_VBOX;
  w->constraint = c;

  // Count children
  size_t count = 0;
  while (children[count] != NULL)
    count++;

  // Copy children array
  w->box.children = arena_alloc(&g_frame_arena, sizeof(Widget *) * count);
  if (!w->box.children)
    return NULL;
  memcpy(w->box.children, children, sizeof(Widget *) * count);
  w->box.count = count;

  return w;
}

Widget *widget_hbox(Constraint c, Widget **children) {
  Widget *w = arena_alloc(&g_frame_arena, sizeof(Widget));
  if (!w)
    return NULL;

  w->type = WIDGET_HBOX;
  w->constraint = c;

  // Count children
  size_t count = 0;
  while (children[count] != NULL)
    count++;

  // Copy children array
  w->box.children = arena_alloc(&g_frame_arena, sizeof(Widget *) * count);
  if (!w->box.children)
    return NULL;
  memcpy(w->box.children, children, sizeof(Widget *) * count);
  w->box.count = count;

  return w;
}

Widget *widget_text(Constraint c, const char *text) {
  Widget *w = arena_alloc(&g_frame_arena, sizeof(Widget));
  if (!w)
    return NULL;

  w->type = WIDGET_TEXT;
  w->constraint = c;
  w->text.text = text;

  return w;
}

Widget *widget_block(Constraint c, const char *title, Widget *child) {
  Widget *w = arena_alloc(&g_frame_arena, sizeof(Widget));
  if (!w)
    return NULL;

  w->type = WIDGET_BLOCK;
  w->constraint = c;
  w->block.title = title;
  w->block.child = child;

  return w;
}

Widget *widget_list(Constraint c, const char **items, const Color *colors,
                    size_t count, size_t selected) {
  Widget *w = arena_alloc(&g_frame_arena, sizeof(Widget));
  if (!w)
    return NULL;

  w->type = WIDGET_LIST;
  w->constraint = c;
  w->list.items = items;
  w->list.colors = colors;
  w->list.count = count;
  w->list.selected = selected;

  return w;
}

Widget *widget_vline(Constraint c) {
  Widget *w = arena_alloc(&g_frame_arena, sizeof(Widget));
  if (!w)
    return NULL;

  w->type = WIDGET_VLINE;
  w->constraint = c;

  return w;
}

Widget *widget_hline(Constraint c) {
  Widget *w = arena_alloc(&g_frame_arena, sizeof(Widget));
  if (!w)
    return NULL;

  w->type = WIDGET_HLINE;
  w->constraint = c;

  return w;
}

Widget *widget_input(Constraint c, const char *text, size_t cursor,
                     const char *prompt) {
  Widget *w = arena_alloc(&g_frame_arena, sizeof(Widget));
  if (!w)
    return NULL;

  w->type = WIDGET_INPUT;
  w->constraint = c;
  w->input.text = text ? text : "";
  w->input.cursor = cursor;
  w->input.prompt = prompt;

  return w;
}

Widget *widget_gauge(Constraint c, double value, const char *label,
                     Color color) {
  Widget *w = arena_alloc(&g_frame_arena, sizeof(Widget));
  if (!w)
    return NULL;

  w->type = WIDGET_GAUGE;
  w->constraint = c;
  w->gauge.value = value < 0.0 ? 0.0 : (value > 1.0 ? 1.0 : value);
  w->gauge.label = label;
  w->gauge.color = color;

  return w;
}

Widget *widget_sparkline(Constraint c, const double *data, size_t count,
                         Color color) {
  Widget *w = arena_alloc(&g_frame_arena, sizeof(Widget));
  if (!w)
    return NULL;

  w->type = WIDGET_SPARKLINE;
  w->constraint = c;
  w->sparkline.data = data;
  w->sparkline.count = count;
  w->sparkline.color = color;

  return w;
}

Widget *widget_table(Constraint c, const char **headers, const char ***rows,
                     size_t col_count, size_t row_count,
                     const uint16_t *widths) {
  Widget *w = arena_alloc(&g_frame_arena, sizeof(Widget));
  if (!w)
    return NULL;

  w->type = WIDGET_TABLE;
  w->constraint = c;
  w->table.headers = headers;
  w->table.rows = rows;
  w->table.col_count = col_count;
  w->table.row_count = row_count;
  w->table.widths = widths;

  return w;
}

Widget *widget_checkbox(Constraint c, const char **items, const int *checked,
                        size_t count, size_t selected) {
  Widget *w = arena_alloc(&g_frame_arena, sizeof(Widget));
  if (!w)
    return NULL;

  w->type = WIDGET_CHECKBOX;
  w->constraint = c;
  w->checkbox.items = items;
  w->checkbox.checked = checked;
  w->checkbox.count = count;
  w->checkbox.selected = selected;

  return w;
}

Widget *widget_progress(Constraint c, double value, const char *label,
                        int show_percent) {
  Widget *w = arena_alloc(&g_frame_arena, sizeof(Widget));
  if (!w)
    return NULL;

  w->type = WIDGET_PROGRESS;
  w->constraint = c;
  w->progress.value = value < 0.0 ? 0.0 : (value > 1.0 ? 1.0 : value);
  w->progress.label = label;
  w->progress.show_percent = show_percent;

  return w;
}

Widget *widget_tabs(Constraint c, const char **labels, size_t count,
                    size_t selected) {
  Widget *w = arena_alloc(&g_frame_arena, sizeof(Widget));
  if (!w)
    return NULL;

  w->type = WIDGET_TABS;
  w->constraint = c;
  w->tabs.labels = labels;
  w->tabs.count = count;
  w->tabs.selected = selected;

  return w;
}

void widget_list_set_selected(Widget *w, size_t selected) {
  if (w && w->type == WIDGET_LIST) {
    w->list.selected = selected;
  }
}

// Rendering

static void render_text(Buffer *buf, Rect area, const char *text) {
  if (rect_is_empty(area) || !text)
    return;

  uint16_t row = 0;
  const char *line_start = text;

  while (*line_start && row < area.height) {
    // Find end of line
    const char *line_end = line_start;
    while (*line_end && *line_end != '\n') {
      line_end++;
    }

    // Copy line to temporary buffer for rendering
    size_t line_len = line_end - line_start;
    char line[256];
    if (line_len > sizeof(line) - 1) {
      line_len = sizeof(line) - 1;
    }
    memcpy(line, line_start, line_len);
    line[line_len] = '\0';

    buffer_set_str(buf, area.y + row, area.x, line);
    row++;

    // Move to next line
    if (*line_end == '\n') {
      line_start = line_end + 1;
    } else {
      break;
    }
  }
}

static void render_block(Buffer *buf, Rect area, const char *title,
                         Widget *child);
static void render_list(Buffer *buf, Rect area, const char **items,
                        const Color *colors, size_t count, size_t selected);

void widget_render(Widget *w, Buffer *buf, Rect area) {
  if (!w || rect_is_empty(area))
    return;

  switch (w->type) {
  case WIDGET_VBOX:
  case WIDGET_HBOX: {
    Direction dir =
        (w->type == WIDGET_VBOX) ? DIRECTION_VERTICAL : DIRECTION_HORIZONTAL;

    size_t n = w->box.count;
    if (n == 0)
      return;

    // Build constraints array from children
    Constraint *constraints =
        arena_alloc(&g_frame_arena, sizeof(Constraint) * n);
    Rect *areas = arena_alloc(&g_frame_arena, sizeof(Rect) * n);
    if (!constraints || !areas)
      return;

    for (size_t i = 0; i < n; i++) {
      constraints[i] = w->box.children[i]->constraint;
    }

    if (layout_split(area, dir, constraints, n, areas) < 0) {
      return; // Layout failed
    }

    // Render children
    for (size_t i = 0; i < n; i++) {
      widget_render(w->box.children[i], buf, areas[i]);
    }
    break;
  }

  case WIDGET_TEXT:
    render_text(buf, area, w->text.text);
    break;

  case WIDGET_BLOCK:
    render_block(buf, area, w->block.title, w->block.child);
    break;

  case WIDGET_LIST:
    render_list(buf, area, w->list.items, w->list.colors, w->list.count,
                w->list.selected);
    break;

  case WIDGET_VLINE: {
    Color line_color = COLOR_INDEX(8); // Gray
    for (uint16_t r = area.y; r < area.y + area.height; r++) {
      buffer_set_cell_styled(buf, r, area.x, '|', line_color,
                             COLOR_DEFAULT_INIT, ATTR_NONE);
    }
    break;
  }

  case WIDGET_HLINE: {
    Color line_color = COLOR_INDEX(8); // Gray
    for (uint16_t c = area.x; c < area.x + area.width; c++) {
      buffer_set_cell_styled(buf, area.y, c, '-', line_color,
                             COLOR_DEFAULT_INIT, ATTR_NONE);
    }
    break;
  }

  case WIDGET_INPUT: {
    // Draw prompt
    size_t prompt_len = 0;
    if (w->input.prompt) {
      prompt_len = strlen(w->input.prompt);
      buffer_set_str_styled(buf, area.y, area.x, w->input.prompt,
                            COLOR_INDEX(14), COLOR_DEFAULT_INIT, ATTR_BOLD);
    }

    // Draw input text
    const char *text = w->input.text ? w->input.text : "";
    size_t text_len = strlen(text);
    size_t available = area.width > prompt_len ? area.width - prompt_len : 0;

    if (text_len > available) {
      // Scroll text to show cursor
      size_t start = 0;
      if (w->input.cursor > available - 1) {
        start = w->input.cursor - available + 1;
      }
      buffer_set_str(buf, area.y, area.x + prompt_len, text + start);
    } else {
      buffer_set_str(buf, area.y, area.x + prompt_len, text);
    }

    // Draw cursor (reverse video)
    size_t cursor_pos = prompt_len + w->input.cursor;
    if (w->input.cursor > strlen(text)) {
      cursor_pos = prompt_len + strlen(text);
    }
    if (cursor_pos < area.width) {
      char cursor_char =
          (w->input.cursor < text_len) ? text[w->input.cursor] : ' ';
      buffer_set_cell_styled(buf, area.y, area.x + cursor_pos, cursor_char,
                             COLOR_INDEX(0), COLOR_INDEX(15), ATTR_NONE);
    }
    break;
  }

  case WIDGET_GAUGE: {
    // Draw label if present
    size_t label_len = 0;
    if (w->gauge.label) {
      label_len = strlen(w->gauge.label) + 1; // +1 for space
      buffer_set_str(buf, area.y, area.x, w->gauge.label);
    }

    // Calculate bar dimensions
    size_t bar_start = area.x + label_len;
    size_t bar_width = area.width > label_len ? area.width - label_len : 0;
    if (bar_width < 3)
      break; // Need at least [=]

    // Draw bar brackets
    buffer_set_cell(buf, area.y, bar_start, '[');
    buffer_set_cell(buf, area.y, bar_start + bar_width - 1, ']');

    // Draw filled portion
    size_t inner_width = bar_width - 2;
    size_t filled = (size_t)(w->gauge.value * inner_width);
    for (size_t i = 0; i < inner_width; i++) {
      char ch = (i < filled) ? '=' : ' ';
      buffer_set_cell_styled(buf, area.y, bar_start + 1 + i, ch, w->gauge.color,
                             COLOR_DEFAULT_INIT, ATTR_NONE);
    }
    break;
  }

  case WIDGET_SPARKLINE: {
    // Sparkline uses Unicode block characters: ▁▂▃▄▅▆▇█
    // For simplicity, use ASCII: _ . - = #
    static const char levels[] = " ._-=*#";
    size_t num_levels = sizeof(levels) - 1;

    if (!w->sparkline.data || w->sparkline.count == 0)
      break;

    size_t data_count = w->sparkline.count;
    size_t display_count = area.width < data_count ? area.width : data_count;
    size_t start_idx =
        data_count > display_count ? data_count - display_count : 0;

    for (size_t i = 0; i < display_count; i++) {
      double val = w->sparkline.data[start_idx + i];
      if (val < 0.0)
        val = 0.0;
      if (val > 1.0)
        val = 1.0;
      size_t level = (size_t)(val * (num_levels - 1));
      buffer_set_cell_styled(buf, area.y, area.x + i, levels[level],
                             w->sparkline.color, COLOR_DEFAULT_INIT, ATTR_NONE);
    }
    break;
  }

  case WIDGET_TABLE: {
    if (!w->table.headers || w->table.col_count == 0)
      break;

    // Calculate column widths
    size_t *col_widths =
        arena_alloc(&g_frame_arena, sizeof(size_t) * w->table.col_count);
    if (!col_widths)
      break;

    for (size_t c = 0; c < w->table.col_count; c++) {
      if (w->table.widths) {
        col_widths[c] = w->table.widths[c];
      } else {
        // Auto-calculate: use header length + 2
        col_widths[c] = strlen(w->table.headers[c]) + 2;
      }
    }

    // Render header
    size_t col_x = area.x;
    for (size_t c = 0; c < w->table.col_count && col_x < area.x + area.width;
         c++) {
      buffer_set_str_styled(buf, area.y, col_x, w->table.headers[c],
                            COLOR_INDEX(14), COLOR_DEFAULT_INIT, ATTR_BOLD);
      col_x += col_widths[c];
    }

    // Render rows
    for (size_t r = 0; r < w->table.row_count && r + 1 < area.height; r++) {
      if (!w->table.rows[r])
        continue;
      col_x = area.x;
      for (size_t c = 0; c < w->table.col_count && col_x < area.x + area.width;
           c++) {
        if (w->table.rows[r][c]) {
          buffer_set_str(buf, area.y + 1 + r, col_x, w->table.rows[r][c]);
        }
        col_x += col_widths[c];
      }
    }
    break;
  }

  case WIDGET_CHECKBOX: {
    if (!w->checkbox.items)
      break;

    for (size_t i = 0; i < w->checkbox.count && i < area.height; i++) {
      const char *item = w->checkbox.items[i];
      if (!item)
        continue;

      int is_checked = w->checkbox.checked && w->checkbox.checked[i];
      int is_selected = (i == w->checkbox.selected);

      // Draw checkbox
      const char *box = is_checked ? "[x] " : "[ ] ";
      Color fg = is_selected ? COLOR_INDEX(0) : COLOR_DEFAULT_INIT;
      Color bg = is_selected ? COLOR_INDEX(14) : COLOR_DEFAULT_INIT;
      uint8_t attrs = is_selected ? ATTR_BOLD : ATTR_NONE;

      // Fill row with background if selected
      if (is_selected) {
        for (uint16_t c = area.x; c < area.x + area.width; c++) {
          buffer_set_cell_styled(buf, area.y + i, c, ' ', fg, bg, attrs);
        }
      }

      // Draw checkbox and label
      buffer_set_str_styled(buf, area.y + i, area.x, box, fg, bg, attrs);
      if (is_checked) {
        // Strikethrough for completed items
        buffer_set_str_styled(buf, area.y + i, area.x + 4, item,
                              COLOR_INDEX(8), bg, attrs);
      } else {
        buffer_set_str_styled(buf, area.y + i, area.x + 4, item, fg, bg, attrs);
      }
    }
    break;
  }

  case WIDGET_PROGRESS: {
    // Draw label if present
    size_t label_len = 0;
    if (w->progress.label) {
      label_len = strlen(w->progress.label) + 1;
      buffer_set_str(buf, area.y, area.x, w->progress.label);
    }

    // Reserve space for percentage if needed
    size_t pct_len = w->progress.show_percent ? 5 : 0; // " 100%"

    // Calculate bar dimensions
    size_t bar_start = area.x + label_len;
    size_t bar_width =
        area.width > label_len + pct_len ? area.width - label_len - pct_len : 0;
    if (bar_width < 3)
      break;

    // Draw bar
    buffer_set_cell(buf, area.y, bar_start, '[');
    buffer_set_cell(buf, area.y, bar_start + bar_width - 1, ']');

    size_t inner_width = bar_width - 2;
    size_t filled = (size_t)(w->progress.value * inner_width);
    for (size_t i = 0; i < inner_width; i++) {
      char ch = (i < filled) ? '#' : '-';
      Color color = (i < filled) ? COLOR_INDEX(10) : COLOR_INDEX(8);
      buffer_set_cell_styled(buf, area.y, bar_start + 1 + i, ch, color,
                             COLOR_DEFAULT_INIT, ATTR_NONE);
    }

    // Draw percentage
    if (w->progress.show_percent) {
      char pct[8];
      snprintf(pct, sizeof(pct), "%3d%%", (int)(w->progress.value * 100));
      buffer_set_str(buf, area.y, bar_start + bar_width + 1, pct);
    }
    break;
  }

  case WIDGET_TABS: {
    if (!w->tabs.labels || w->tabs.count == 0)
      break;

    size_t x = area.x;
    for (size_t i = 0; i < w->tabs.count && x < area.x + area.width; i++) {
      const char *label = w->tabs.labels[i];
      if (!label)
        continue;

      size_t label_len = strlen(label);
      int is_selected = (i == w->tabs.selected);

      // Draw tab with padding
      if (is_selected) {
        // Selected tab: highlighted
        buffer_set_cell_styled(buf, area.y, x, '[', COLOR_INDEX(14),
                               COLOR_DEFAULT_INIT, ATTR_BOLD);
        for (size_t j = 0; j < label_len && x + 1 + j < area.x + area.width;
             j++) {
          buffer_set_cell_styled(buf, area.y, x + 1 + j, label[j],
                                 COLOR_INDEX(14), COLOR_DEFAULT_INIT,
                                 ATTR_BOLD);
        }
        if (x + 1 + label_len < area.x + area.width) {
          buffer_set_cell_styled(buf, area.y, x + 1 + label_len, ']',
                                 COLOR_INDEX(14), COLOR_DEFAULT_INIT,
                                 ATTR_BOLD);
        }
      } else {
        // Unselected tab: dimmed
        buffer_set_cell_styled(buf, area.y, x, ' ', COLOR_INDEX(8),
                               COLOR_DEFAULT_INIT, ATTR_NONE);
        for (size_t j = 0; j < label_len && x + 1 + j < area.x + area.width;
             j++) {
          buffer_set_cell_styled(buf, area.y, x + 1 + j, label[j],
                                 COLOR_INDEX(8), COLOR_DEFAULT_INIT, ATTR_NONE);
        }
        if (x + 1 + label_len < area.x + area.width) {
          buffer_set_cell_styled(buf, area.y, x + 1 + label_len, ' ',
                                 COLOR_INDEX(8), COLOR_DEFAULT_INIT, ATTR_NONE);
        }
      }

      x += label_len + 3; // label + brackets/spaces + separator
    }
    break;
  }
  }
}

static void render_block(Buffer *buf, Rect area, const char *title,
                         Widget *child) {
  if (rect_is_empty(area))
    return;

  Color border_color = COLOR_INDEX(8); // Gray

  // Draw top border
  buffer_set_cell_styled(buf, area.y, area.x, '+', border_color,
                         COLOR_DEFAULT_INIT, ATTR_NONE);
  for (uint16_t c = area.x + 1; c < area.x + area.width - 1; c++) {
    buffer_set_cell_styled(buf, area.y, c, '-', border_color,
                           COLOR_DEFAULT_INIT, ATTR_NONE);
  }
  if (area.width > 1) {
    buffer_set_cell_styled(buf, area.y, area.x + area.width - 1, '+',
                           border_color, COLOR_DEFAULT_INIT, ATTR_NONE);
  }

  // Draw title
  if (title && area.width > 4) {
    size_t title_len = strlen(title);
    size_t max_title = area.width - 4;
    if (title_len > max_title)
      title_len = max_title;

    buffer_set_cell_styled(buf, area.y, area.x + 1, ' ', COLOR_DEFAULT_INIT,
                           COLOR_DEFAULT_INIT, ATTR_NONE);
    for (size_t i = 0; i < title_len; i++) {
      buffer_set_cell_styled(buf, area.y, area.x + 2 + i, title[i],
                             COLOR_INDEX(11), COLOR_DEFAULT_INIT, ATTR_BOLD);
    }
    buffer_set_cell_styled(buf, area.y, area.x + 2 + title_len, ' ',
                           COLOR_DEFAULT_INIT, COLOR_DEFAULT_INIT, ATTR_NONE);
  }

  // Draw side borders
  for (uint16_t r = area.y + 1; r < area.y + area.height - 1; r++) {
    buffer_set_cell_styled(buf, r, area.x, '|', border_color,
                           COLOR_DEFAULT_INIT, ATTR_NONE);
    if (area.width > 1) {
      buffer_set_cell_styled(buf, r, area.x + area.width - 1, '|', border_color,
                             COLOR_DEFAULT_INIT, ATTR_NONE);
    }
  }

  // Draw bottom border
  if (area.height > 1) {
    uint16_t bottom = area.y + area.height - 1;
    buffer_set_cell_styled(buf, bottom, area.x, '+', border_color,
                           COLOR_DEFAULT_INIT, ATTR_NONE);
    for (uint16_t c = area.x + 1; c < area.x + area.width - 1; c++) {
      buffer_set_cell_styled(buf, bottom, c, '-', border_color,
                             COLOR_DEFAULT_INIT, ATTR_NONE);
    }
    if (area.width > 1) {
      buffer_set_cell_styled(buf, bottom, area.x + area.width - 1, '+',
                             border_color, COLOR_DEFAULT_INIT, ATTR_NONE);
    }
  }

  // Render child in inner area
  if (child && area.width > 2 && area.height > 2) {
    Rect inner = {.x = area.x + 1,
                  .y = area.y + 1,
                  .width = area.width - 2,
                  .height = area.height - 2};
    widget_render(child, buf, inner);
  }
}

static void render_list(Buffer *buf, Rect area, const char **items,
                        const Color *colors, size_t count, size_t selected) {
  if (rect_is_empty(area) || !items)
    return;

  for (size_t i = 0; i < count && i < area.height; i++) {
    const char *item = items[i];
    if (!item)
      continue;

    int is_selected = (i == selected);
    Color item_color =
        (colors && colors[i].type != COLOR_DEFAULT)
            ? colors[i]
            : COLOR_DEFAULT_INIT;
    Color fg = is_selected ? COLOR_INDEX(0) : item_color;
    Color bg = is_selected ? COLOR_INDEX(14) : COLOR_DEFAULT_INIT;
    uint8_t attrs = is_selected ? ATTR_BOLD : ATTR_NONE;

    // Fill row with background if selected
    if (is_selected) {
      for (uint16_t c = area.x; c < area.x + area.width; c++) {
        buffer_set_cell_styled(buf, area.y + i, c, ' ', fg, bg, attrs);
      }
    }

    // Draw item text (tab expansion handled by buffer_set_str_styled)
    buffer_set_str_styled(buf, area.y + i, area.x, item, fg, bg, attrs);
  }
}
