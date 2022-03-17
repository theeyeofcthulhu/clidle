# Clidle - Wordle in the terminal

## Description

This is a small implementation of the popular puzzle game Wordle for terminals.
It uses coloring to emulate the indicators for the letters and displays a colored
alphabet at the bottom to emulate the virtual keyboard in Wordle.

## Requirements

- [GNU readline](https://tiswww.cwru.edu/php/chet/readline/rltop.html)
- Terminal support for ANSI and VT100 escape sequences

## Compilation

```console
$ make
```

## Quick Start

```console
$ ./clidle
```

Refer to the GNU readline documentation for line editing controls. GNU readline starts in vi-mode
in this program.

For the rules of wordle refer to their [website](https://www.nytimes.com/games/wordle/index.html).
Words have to be five letters long and appear in words.txt.

Have fun!

## Terminals

Terminals I have successfully tested this on:

- Alacritty
- Kitty
- XTerm
- st
- Konsole
- MSYS2

## Licensing

This project is licensed under the GPLv3.

The included [string view library](https://github.com/theeyeofcthulhu/sv) by your's truly is licensed under the MIT License.
