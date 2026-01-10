#include "widget.h"
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

Widget *widget_list(Constraint c, const char **items, size_t count,
                    size_t selected) {
  Widget *w = arena_alloc(&g_frame_arena, sizeof(Widget));
  if (!w)
    return NULL;

  w->type = WIDGET_LIST;
  w->constraint = c;
  w->list.items = items;
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
                        size_t count, size_t selected);

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
    render_list(buf, area, w->list.items, w->list.count, w->list.selected);
    break;

  case WIDGET_VLINE: {
    Color line_color = COLOR_INDEX(8);  // Gray
    for (uint16_t r = area.y; r < area.y + area.height; r++) {
      buffer_set_cell_styled(buf, r, area.x, '|', line_color,
                             COLOR_DEFAULT_INIT, ATTR_NONE);
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
                        size_t count, size_t selected) {
  if (rect_is_empty(area) || !items)
    return;

  for (size_t i = 0; i < count && i < area.height; i++) {
    const char *item = items[i];
    if (!item)
      continue;

    int is_selected = (i == selected);
    Color fg = is_selected ? COLOR_INDEX(0) : COLOR_DEFAULT_INIT;
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
