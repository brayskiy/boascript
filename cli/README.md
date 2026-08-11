# boa — interactive BoaScript interpreter

`boa` is a Python-style REPL for BoaScript. It keeps a persistent session, so
variables, functions, and arrays defined on one line remain in effect on the
next.

## Build & run

From the repository root:

```sh
make cli           # builds the library and the REPL
./cli/boa
```

or, once the library exists in `../distribution`:

```sh
cd cli && make run
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
printf 'a = 0\na + 2\nexit()\n' | ./cli/boa
```
