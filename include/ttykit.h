#ifndef TTYKIT_H
#define TTYKIT_H

// Terminal raw mode
int tty_enable_raw_mode(void);
void tty_disable_raw_mode(void);

// Alternate screen
void tty_enter_alternate_screen(void);
void tty_leave_alternate_screen(void);

#endif // TTYKIT_H
