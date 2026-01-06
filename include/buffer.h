#ifndef TTYKIT_BUFFER_H
#define TTYKIT_BUFFER_H

#include <stddef.h>

typedef struct {
    char ch;
    // TODO: add fg, bg, attrs
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

#endif // TTYKIT_BUFFER_H
