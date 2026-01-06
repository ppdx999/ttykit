#include "ttykit.h"
#include <termios.h>
#include <unistd.h>

static struct termios orig_termios;
static int raw_mode_enabled = 0;

int tty_enable_raw_mode(void) {
    if (raw_mode_enabled) return 0;
    if (!isatty(STDIN_FILENO)) return -1;

    if (tcgetattr(STDIN_FILENO, &orig_termios) == -1) return -1;

    struct termios raw = orig_termios;

    // Input: no break, no CR to NL, no parity check, no strip, no flow ctrl
    raw.c_iflag &= ~(BRKINT | ICRNL | INPCK | ISTRIP | IXON);

    // Output: disable post processing
    raw.c_oflag &= ~(OPOST);

    // Control: set 8 bit chars
    raw.c_cflag |= (CS8);

    // Local: no echo, no canonical, no extended, no signal
    raw.c_lflag &= ~(ECHO | ICANON | IEXTEN | ISIG);

    // Return each byte, no timeout
    raw.c_cc[VMIN] = 1;
    raw.c_cc[VTIME] = 0;

    if (tcsetattr(STDIN_FILENO, TCSAFLUSH, &raw) == -1) return -1;

    raw_mode_enabled = 1;
    return 0;
}

void tty_disable_raw_mode(void) {
    if (raw_mode_enabled) {
        tcsetattr(STDIN_FILENO, TCSAFLUSH, &orig_termios);
        raw_mode_enabled = 0;
    }
}

void tty_enter_alternate_screen(void) {
    write(STDOUT_FILENO, "\x1b[?1049h", 8);
}

void tty_leave_alternate_screen(void) {
    write(STDOUT_FILENO, "\x1b[?1049l", 8);
}
