# TUI Widget Roadmap

## Current Widgets
- VBOX / HBOX - Layout containers
- TEXT - Text display
- BLOCK - Bordered container with title
- LIST / LIST_COLORED - Selectable list with optional colors
- VLINE - Vertical line separator

## Next: Interactive Search (fzf-style)

### New Widgets Required
1. **HLINE** - Horizontal line separator
   - Simple horizontal divider
   - Matches VLINE for consistency

2. **INPUT** - Text input field
   - Single-line text input
   - Cursor position tracking
   - Character insertion/deletion

### App: examples/finder.c
```
+---------------------------+
| > query_here|             |  <- INPUT
+---------------------------+
| file1.txt                 |  <- LIST (filtered)
| file2.c                   |
| another_file.h            |
+---------------------------+
| 3/100 matches             |  <- TEXT (status)
+---------------------------+
```

### Features
- Real-time filtering as user types
- j/k or arrow keys to navigate results
- Enter to select
- Esc or q to quit

## Future Ideas

### System Monitor (htop-style)
- GAUGE - Progress bar / meter
- SPARKLINE - Mini line graph
- TABLE - Tabular data display

### Task Manager
- CHECKBOX - Toggle checkbox
- PROGRESS - Progress bar

### Git Client (lazygit-style)
- TABS - Tab switching
- TABLE - Commit history
