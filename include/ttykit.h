#ifndef TTYKIT_H
#define TTYKIT_H

// Terminal raw mode
int tty_enable_raw_mode(void);
void tty_disable_raw_mode(void);

// Alternate screen
void tty_enter_alternate_screen(void);
void tty_leave_alternate_screen(void);

// Cursor control
void tty_cursor_hide(void);
void tty_cursor_show(void);
void tty_cursor_move(int row, int col);
void tty_cursor_home(void);

// Screen
void tty_clear_screen(void);
int tty_get_size(int *rows, int *cols);

#endif // TTYKIT_H
