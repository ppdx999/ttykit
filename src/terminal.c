#include "ttykit.h"
#include <fcntl.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <termios.h>
#include <unistd.h>

static struct termios orig_termios;
static int raw_mode_enabled = 0;
static int tty_fd = -1;

int tty_get_fd(void) { return tty_fd; }

int tty_enable_raw_mode(void) {
  if (raw_mode_enabled)
    return 0;

  // Open /dev/tty directly to handle piped stdin
  tty_fd = open("/dev/tty", O_RDWR);
  if (tty_fd == -1)
    return -1;

  if (tcgetattr(tty_fd, &orig_termios) == -1) {
    close(tty_fd);
    tty_fd = -1;
    return -1;
  }

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

  if (tcsetattr(tty_fd, TCSAFLUSH, &raw) == -1) {
    close(tty_fd);
    tty_fd = -1;
    return -1;
  }

  raw_mode_enabled = 1;
  return 0;
}

void tty_disable_raw_mode(void) {
  if (raw_mode_enabled) {
    tcsetattr(tty_fd, TCSAFLUSH, &orig_termios);
    close(tty_fd);
    tty_fd = -1;
    raw_mode_enabled = 0;
  }
}

void tty_enter_alternate_screen(void) {
  write(STDOUT_FILENO, "\x1b[?1049h", 8);
}

void tty_leave_alternate_screen(void) {
  write(STDOUT_FILENO, "\x1b[?1049l", 8);
}

void tty_cursor_hide(void) { write(STDOUT_FILENO, "\x1b[?25l", 6); }

void tty_cursor_show(void) { write(STDOUT_FILENO, "\x1b[?25h", 6); }

void tty_cursor_move(int row, int col) {
  char buf[32];
  int len = snprintf(buf, sizeof(buf), "\x1b[%d;%dH", row, col);
  write(STDOUT_FILENO, buf, len);
}

void tty_cursor_home(void) { write(STDOUT_FILENO, "\x1b[H", 3); }

void tty_clear_screen(void) { write(STDOUT_FILENO, "\x1b[2J", 4); }

int tty_get_size(int *rows, int *cols) {
  struct winsize ws;
  if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == -1)
    return -1;
  if (rows)
    *rows = ws.ws_row;
  if (cols)
    *cols = ws.ws_col;
  return 0;
}
