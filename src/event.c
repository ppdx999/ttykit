#include "event.h"
#include "ttykit.h"
#include <signal.h>
#include <string.h>
#include <sys/select.h>
#include <unistd.h>

static volatile sig_atomic_t resize_pending = 0;

static void sigwinch_handler(int sig) {
    (void)sig;
    resize_pending = 1;
}

int event_init(void) {
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = sigwinch_handler;
    sa.sa_flags = SA_RESTART;
    if (sigaction(SIGWINCH, &sa, NULL) == -1) {
        return -1;
    }
    return 0;
}

void event_cleanup(void) {
    signal(SIGWINCH, SIG_DFL);
}

// Parse escape sequence into key event
// Returns number of bytes consumed, or 0 if incomplete
static int parse_escape_seq(const char *buf, int len, KeyEvent *key) {
    if (len < 2) return 0;

    // ESC [ sequences (CSI)
    if (buf[1] == '[') {
        if (len < 3) return 0;

        // Arrow keys and simple sequences
        switch (buf[2]) {
            case 'A': key->code = KEY_UP; return 3;
            case 'B': key->code = KEY_DOWN; return 3;
            case 'C': key->code = KEY_RIGHT; return 3;
            case 'D': key->code = KEY_LEFT; return 3;
            case 'H': key->code = KEY_HOME; return 3;
            case 'F': key->code = KEY_END; return 3;
        }

        // Extended sequences: ESC [ <num> ~
        if (buf[2] >= '0' && buf[2] <= '9') {
            if (len < 4) return 0;
            if (buf[3] == '~') {
                switch (buf[2]) {
                    case '1': key->code = KEY_HOME; return 4;
                    case '2': key->code = KEY_INSERT; return 4;
                    case '3': key->code = KEY_DELETE; return 4;
                    case '4': key->code = KEY_END; return 4;
                    case '5': key->code = KEY_PAGE_UP; return 4;
                    case '6': key->code = KEY_PAGE_DOWN; return 4;
                    case '7': key->code = KEY_HOME; return 4;
                    case '8': key->code = KEY_END; return 4;
                }
            }
            // Function keys: ESC [ 1 <num> ~
            if (buf[2] == '1' && len >= 5 && buf[4] == '~') {
                switch (buf[3]) {
                    case '5': key->code = KEY_F5; return 5;
                    case '7': key->code = KEY_F6; return 5;
                    case '8': key->code = KEY_F7; return 5;
                    case '9': key->code = KEY_F8; return 5;
                }
            }
            if (buf[2] == '2' && len >= 5 && buf[4] == '~') {
                switch (buf[3]) {
                    case '0': key->code = KEY_F9; return 5;
                    case '1': key->code = KEY_F10; return 5;
                    case '3': key->code = KEY_F11; return 5;
                    case '4': key->code = KEY_F12; return 5;
                }
            }
        }
    }

    // ESC O sequences (SS3) - F1-F4
    if (buf[1] == 'O') {
        if (len < 3) return 0;
        switch (buf[2]) {
            case 'P': key->code = KEY_F1; return 3;
            case 'Q': key->code = KEY_F2; return 3;
            case 'R': key->code = KEY_F3; return 3;
            case 'S': key->code = KEY_F4; return 3;
        }
    }

    // Unknown escape sequence - treat as bare ESC + Alt
    key->code = KEY_CHAR;
    key->ch = buf[1];
    key->mod |= MOD_ALT;
    return 2;
}

// Parse a single key from input buffer
static int parse_key(const char *buf, int len, KeyEvent *key) {
    memset(key, 0, sizeof(*key));

    if (len == 0) return 0;

    unsigned char c = buf[0];

    // Escape sequence
    if (c == 0x1b) {
        if (len == 1) {
            // Bare escape
            key->code = KEY_ESCAPE;
            return 1;
        }
        return parse_escape_seq(buf, len, key);
    }

    // Control characters
    if (c < 32) {
        switch (c) {
            case 0x0d:  // CR
                key->code = KEY_ENTER;
                return 1;
            case 0x09:  // TAB
                key->code = KEY_TAB;
                return 1;
            case 0x7f:  // DEL (some terminals send this for backspace)
                key->code = KEY_BACKSPACE;
                return 1;
            case 0x08:  // BS
                key->code = KEY_BACKSPACE;
                return 1;
            default:
                // Ctrl+letter (Ctrl+A = 1, Ctrl+B = 2, etc.)
                key->code = KEY_CHAR;
                key->ch = c + 'a' - 1;
                key->mod = MOD_CTRL;
                return 1;
        }
    }

    // DEL character (backspace on some terminals)
    if (c == 0x7f) {
        key->code = KEY_BACKSPACE;
        return 1;
    }

    // Regular character
    key->code = KEY_CHAR;
    key->ch = c;
    return 1;
}

int event_poll(Event *event, int timeout_ms) {
    memset(event, 0, sizeof(*event));

    // Check for pending resize
    if (resize_pending) {
        resize_pending = 0;
        event->type = EVENT_RESIZE;
        tty_get_size(&event->resize.rows, &event->resize.cols);
        return 1;
    }

    // Set up select for input with timeout
    fd_set fds;
    FD_ZERO(&fds);
    FD_SET(STDIN_FILENO, &fds);

    struct timeval tv;
    struct timeval *tvp = NULL;
    if (timeout_ms >= 0) {
        tv.tv_sec = timeout_ms / 1000;
        tv.tv_usec = (timeout_ms % 1000) * 1000;
        tvp = &tv;
    }

    int ret = select(STDIN_FILENO + 1, &fds, NULL, NULL, tvp);

    // Check resize again (signal may have interrupted select)
    if (resize_pending) {
        resize_pending = 0;
        event->type = EVENT_RESIZE;
        tty_get_size(&event->resize.rows, &event->resize.cols);
        return 1;
    }

    if (ret == -1) {
        return -1;
    }
    if (ret == 0) {
        event->type = EVENT_NONE;
        return 0;
    }

    // Read input
    char buf[16];
    int len = read(STDIN_FILENO, buf, sizeof(buf));
    if (len <= 0) {
        return -1;
    }

    // Parse key
    event->type = EVENT_KEY;
    parse_key(buf, len, &event->key);
    return 1;
}
