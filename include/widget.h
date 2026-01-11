#ifndef TTYKIT_WIDGET_H
#define TTYKIT_WIDGET_H

#include "buffer.h"
#include "layout.h"
#include <stddef.h>
#include <stdint.h>

// Widget types
typedef enum {
  WIDGET_VBOX,
  WIDGET_HBOX,
  WIDGET_TEXT,
  WIDGET_BLOCK,
  WIDGET_LIST,
  WIDGET_VLINE
} WidgetType;

// Forward declaration
typedef struct Widget Widget;

// Widget structure
struct Widget {
  WidgetType type;
  Constraint constraint;
  union {
    struct {
      Widget **children;
      size_t count;
    } box;
    struct {
      const char *text;
    } text;
    struct {
      const char *title;
      Widget *child;
    } block;
    struct {
      const char **items;
      const Color *colors; // Optional per-item foreground colors (NULL = default)
      size_t count;
      size_t selected;
    } list;
  };
};

// Constraint macros (use as first argument)
#define LEN(n) ((Constraint){CONSTRAINT_LENGTH, (n), 0})
#define PCT(n) ((Constraint){CONSTRAINT_PERCENT, (n), 0})
#define FILL ((Constraint){CONSTRAINT_FILL, 1, 0})
#define MIN(n) ((Constraint){CONSTRAINT_MIN, (n), 0})
#define MAX(n) ((Constraint){CONSTRAINT_MAX, (n), 0})

// Layout widgets
#define VBOX(c, ...) widget_vbox((c), (Widget *[]){__VA_ARGS__, NULL})
#define HBOX(c, ...) widget_hbox((c), (Widget *[]){__VA_ARGS__, NULL})

// Content widgets
#define TEXT(c, s) widget_text((c), (s))
#define BLOCK(c, t, ch) widget_block((c), (t), (ch))
#define LIST(c, i, n, s) widget_list((c), (i), NULL, (n), (s))
#define LIST_COLORED(c, i, colors, n, s) widget_list((c), (i), (colors), (n), (s))
#define VLINE(c) widget_vline((c))

// Frame arena management
void ui_frame_begin(void);
void ui_frame_end(void);

// Widget constructors (use macros instead)
Widget *widget_vbox(Constraint c, Widget **children);
Widget *widget_hbox(Constraint c, Widget **children);
Widget *widget_text(Constraint c, const char *text);
Widget *widget_block(Constraint c, const char *title, Widget *child);
Widget *widget_list(Constraint c, const char **items, const Color *colors,
                    size_t count, size_t selected);
Widget *widget_vline(Constraint c);

// Set selected index for list widget
void widget_list_set_selected(Widget *w, size_t selected);

// Rendering
void widget_render(Widget *w, Buffer *buf, Rect area);

#endif // TTYKIT_WIDGET_H
