# ttykit

ttykit is a lightweight TUI toolkit written in C.
It renders terminal UIs using a diff-based cell buffer
and talks directly to POSIX ttys via ANSI escape sequences.

## Features
- Cell-based screen buffer with diff rendering (minimal terminal output)
- POSIX tty backend (termios, ANSI escape sequences)
- Layout: split areas with constraints (percent/length/min/fill)
- Widgets: Block, Paragraph, List, Gauge (WIP)
- Events: key input, resize (SIGWINCH), optional tick
- TrueColor/256-color styling (WIP)

## Non-goals (for now)
- Full Unicode width/grapheme cluster correctness
- Mouse support
- Windows console backend
