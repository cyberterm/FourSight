# FourSight

**FourSight** is a strong Connect 4 engine designed for strategic gameplay. It supports various commands for interaction and provides flexibility for players and developers alike.

## Supported Commands

The following commands are supported by FourSight:

- **2players**: Play with a friend (No AI).
- **go**: Search (current position) indefinitely.
- **go depth `<d>`**: Search to depth `<d>` (e.g., `go depth 20`).
- **go movetime `<s>`**: Search for `<s>` milliseconds (e.g., `go movetime 20`).
  - (The last depth calculation will continue over this time so you need to account for that.)
- **position startpos**: Clear the board.
- **position `<text>`**: Insert a position as a string.  
  Example: `position 2X4/2O4/7/7/7/7` adds an X and an O to the third column. Numbers count spaces and must be accurate(!).  
  Dashes or other characters are just visual aids.
- **add `<i>`**: Add integer `<i>` to the internal state.
- **play**: Play the best move with default settings.
- **play depth `<d>`**: Play the best move searching to depth `<d>`.
- **play movetime `<s>`**: Play the best move searching for `<s>` milliseconds.
- **print**: Print the current state.
- **help**: Show this help message.

**In-game controls**:
- Enter numbers `1-7` to play a column.
- Enter `r` to restart or `q` to quit.

## Installation

To get started with FourSight, clone or download this repository:

```bash
git clone https://github.com/cyberterm/FourSight.git

You can use the provided make file to compile:
```bash
make release

(a debug build is also provided with make)

Or run with gcc from the main directory:

```bash
gcc -O3 -march=native -Iinclude src/*.c -o bin/connect4_release
