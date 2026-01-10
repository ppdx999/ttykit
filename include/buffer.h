#ifndef TTYKIT_BUFFER_H
#define TTYKIT_BUFFER_H

#include <stddef.h>
#include <stdint.h>

// Color type
typedef enum {
  COLOR_DEFAULT = 0, // Terminal default color
  COLOR_INDEXED,     // 256-color palette (0-255)
  COLOR_RGB          // TrueColor (24-bit)
} ColorType;

typedef struct {
  ColorType type;
  uint8_t r, g, b; // Used for RGB; 'r' doubles as index for INDEXED
} Color;

// Text attributes (bitmask)
typedef enum {
  ATTR_NONE = 0,
  ATTR_BOLD = 1 << 0,
  ATTR_DIM = 1 << 1,
  ATTR_ITALIC = 1 << 2,
  ATTR_UNDERLINE = 1 << 3,
  ATTR_BLINK = 1 << 4,
  ATTR_REVERSE = 1 << 5,
  ATTR_HIDDEN = 1 << 6,
  ATTR_STRIKE = 1 << 7
} Attr;

// Helper macros for creating colors
#define COLOR_DEFAULT_INIT ((Color){COLOR_DEFAULT, 0, 0, 0})
#define COLOR_INDEX(i) ((Color){COLOR_INDEXED, (i), 0, 0})
#define COLOR_RGB_INIT(rv, gv, bv) ((Color){COLOR_RGB, (rv), (gv), (bv)})

typedef struct {
  char ch;
  Color fg;
  Color bg;
  uint8_t attrs;
} Cell;

typedef struct {
  Cell *cells;
  int rows;
  int cols;
} Buffer;

Buffer *buffer_create(int rows, int cols);
void buffer_destroy(Buffer *buf);
void buffer_clear(Buffer *buf);
void buffer_set_cell(Buffer *buf, int row, int col, char ch);
Cell *buffer_get_cell(Buffer *buf, int row, int col);
void buffer_set_str(Buffer *buf, int row, int col, const char *str);
void buffer_render(Buffer *buf);

// Styled cell operations
void buffer_set_cell_styled(Buffer *buf, int row, int col, char ch, Color fg,
                            Color bg, uint8_t attrs);
void buffer_set_str_styled(Buffer *buf, int row, int col, const char *str,
                           Color fg, Color bg, uint8_t attrs);

#endif // TTYKIT_BUFFER_H
