#ifndef TTYKIT_EVENT_H
#define TTYKIT_EVENT_H

#include <stdint.h>

// Event types
typedef enum {
  EVENT_NONE = 0, // No event (timeout)
  EVENT_KEY,      // Key press
  EVENT_RESIZE    // Terminal resize
} EventType;

// Special keys
typedef enum {
  KEY_CHAR = 0, // Regular character (use event.key.ch)

  // Arrow keys
  KEY_UP,
  KEY_DOWN,
  KEY_LEFT,
  KEY_RIGHT,

  // Navigation
  KEY_HOME,
  KEY_END,
  KEY_PAGE_UP,
  KEY_PAGE_DOWN,
  KEY_INSERT,
  KEY_DELETE,

  // Control keys
  KEY_ENTER,
  KEY_TAB,
  KEY_BACKSPACE,
  KEY_ESCAPE,

  // Function keys
  KEY_F1,
  KEY_F2,
  KEY_F3,
  KEY_F4,
  KEY_F5,
  KEY_F6,
  KEY_F7,
  KEY_F8,
  KEY_F9,
  KEY_F10,
  KEY_F11,
  KEY_F12
} KeyCode;

// Modifier flags
typedef enum {
  MOD_NONE = 0,
  MOD_CTRL = 1 << 0,
  MOD_ALT = 1 << 1,
  MOD_SHIFT = 1 << 2
} KeyMod;

// Key event data
typedef struct {
  KeyCode code; // KEY_CHAR for regular chars, or special key
  char ch;      // Character (valid when code == KEY_CHAR)
  uint8_t mod;  // Modifier flags (MOD_CTRL, MOD_ALT, etc.)
} KeyEvent;

// Resize event data
typedef struct {
  int rows;
  int cols;
} ResizeEvent;

// Unified event structure
typedef struct {
  EventType type;
  union {
    KeyEvent key;
    ResizeEvent resize;
  };
} Event;

// Initialize event system (sets up signal handlers)
int event_init(void);

// Clean up event system
void event_cleanup(void);

// Poll for next event
// timeout_ms: -1 = block forever, 0 = non-blocking, >0 = timeout in ms
// Returns 1 if event received, 0 on timeout, -1 on error
int event_poll(Event *event, int timeout_ms);

#endif // TTYKIT_EVENT_H
