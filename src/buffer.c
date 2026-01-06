#include "buffer.h"
#include "ttykit.h"
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

Buffer *buffer_create(int rows, int cols) {
    Buffer *buf = malloc(sizeof(Buffer));
    if (!buf) return NULL;

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
    }
}

void buffer_set_str(Buffer *buf, int row, int col, const char *str) {
    while (*str && col < buf->cols) {
        buffer_set_cell(buf, row, col, *str);
        str++;
        col++;
    }
}

void buffer_render(Buffer *buf) {
    tty_cursor_home();

    // Build output in a single buffer for efficiency
    char *out = malloc(buf->rows * buf->cols + buf->rows);
    if (!out) return;

    int pos = 0;
    for (int r = 0; r < buf->rows; r++) {
        for (int c = 0; c < buf->cols; c++) {
            out[pos++] = buf->cells[r * buf->cols + c].ch;
        }
    }

    write(STDOUT_FILENO, out, pos);
    free(out);
}
