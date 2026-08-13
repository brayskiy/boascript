# boa — interactive BoaScript interpreter

`boa` is a Python-style REPL for BoaScript. It keeps a persistent session, so
variables, functions, and arrays defined on one line remain in effect on the
next.

## Build & run

From the repository root:

```sh
make cli           # builds the library and the REPL
./apps/cli/boa
```

or, once the library exists in `../../distribution`:

```sh
cd apps/cli && make run
```

## Using it

```
BoaScript 26.33.01 on linux
BoaScript - a light, C-style scripting language and calculator
Type "exit()" or Ctrl-D (i.e. EOF) to exit.
>>> a = 0
>>> a + 2
2
>>> func sq(n) {
...     return n * n;
... }
>>> sq(7)
49
>>> exit()
```

* A **bare expression** is echoed — its value is printed (`a + 2` shows `2`).
* A **statement** (assignment, `print`, `if`, `while`, `for`, `func`, `case`,
  ...) runs silently.
* A trailing `;` is optional and added automatically where needed.
* **Unbalanced** `{ ( [` continue on a `...` prompt until closed.
* **`exit()`** (or Ctrl-D / EOF) exits; a bare `exit` prints a reminder.

### Line editing

A small built-in editor (no external dependency) provides:

* **Up / Down** arrows — recall the previous / next input from history.
* **Left / Right** arrows, **Home** / **End** (or Ctrl-A / Ctrl-E) — move the
  cursor.
* **Backspace** and **Delete** — edit in place.
* **Ctrl-C** — cancel the current line; **Ctrl-D** on an empty line exits.

It can also be scripted by piping input:

```sh
printf 'a = 0\na + 2\nexit()\n' | ./apps/cli/boa
```

## Portability

`boa` builds from a single source on every supported platform; the line
editor has two backends selected at compile time:

* **Linux and macOS** — a POSIX backend (`termios` raw mode + ANSI escape
  sequences). One source, one binary; no external dependency (in particular
  not GNU readline, whose GPL license would conflict with this project's MIT
  license). Build with `make cli`.
* **Windows** (native, Windows 10 or newer) — a Windows Console backend
  (`ReadConsoleInput` + virtual-terminal output via
  `ENABLE_VIRTUAL_TERMINAL_PROCESSING`), compiled in behind `#ifdef _WIN32`.
  On Windows the EOF key is **Ctrl-Z**. Build with MinGW-w64 / MSYS2 using
  the same Makefiles, or compile directly, e.g.:

  ```sh
  g++ -O2 -DNDEBUG -DCALC_BATCH -I../../distribution -I../../extras \
      boa.cpp -L../../distribution -lBoascript -o boa.exe
  ```

  Under WSL or Cygwin the POSIX backend is used instead.

When stdin is not a terminal (piped input) both backends fall back to plain
line reading, so scripting works everywhere.
