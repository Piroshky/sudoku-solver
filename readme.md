# Sudoku Solver

I figured that if I'm going to ~~cheat~~ get hints on my sudoku puzzles I might as well get them from a program I made myself.

To build just run the makefile.

To use run
```
$ ss yoursudokufile [options]
```
where yoursudokufile is a file where each line represents a row in the puzzle, and spaces represent unfilled cells. See repository for examples. If there aren't nine characters in the line then the program will assume the remaining cells are all empty.
