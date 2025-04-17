# FourSight

**FourSight** is a strong Connect 4 engine that can precisely analyze a position to a great depth but also provide good intuition in harder positions that can't be exhaustively analyzed. Human opponents might struggle to defeat even analysis depth 2!

## Supported Commands

Use the following commands to interact:

- **2players**: Start a two player game with a friend(No AI).
- **go**: Search (current position) indefinitely.
- **go depth `<d>`**: Search to depth `<d>` (e.g., `go depth 20`).
- **go movetime `<s>`**: Search for `<s>` milliseconds (e.g., `go movetime 20000` searches for 20 seconds).
  - (The last depth calculation will continue over this time so you need to account for that.)
- **position startpos**: Clear the board.
- **position `<text>`**: Insert a position as a string.  
  Example: `position 2X4/2O4/7/7/7/7` adds an X and an O to the third column. Numbers count spaces and must be accurate(!).  
  Dashes or other characters are just visual aids.
- **add `<ijk...>`**: Add integers `i, j, k ...` to internal state in one go.
  Example: `add 445` will add to column `4` then `4` again then `5` in one go (any other character is ignored)"
- **play**: Start a game with the AI with default settings.
- **play depth `<d>`**: Start a game with the AI and make it search up to depth `<d>`.
- **play movetime `<s>`**: Start a game with the AI and make it search for `<s>` milliseconds.
- **print**: Print the current state.
- **help**: Show this help message.

**In-game controls**:
- Enter numbers `1-7` to play a column.
- Enter `r` to restart or `q` to quit.

## Installation

To get started with FourSight, clone or download this repository:

```bash
git clone https://github.com/cyberterm/FourSight.git
```

You can use the provided makefile to compile:
```bash
make release
```

(a debug build is also provided with make)

Or directly compile with gcc from the main directory:

```bash
gcc -O3 -march=native -Iinclude src/*.c -o FourSight
```