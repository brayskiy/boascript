# BoaScript CLI demo

`boademo` is an interactive menu of BoaScript feature demos. Each item shows
the BoaScript source it runs and the output produced by `Boascript::run()`.

## Build & run

From the repository root:

```sh
make demo          # builds the library and the demo
./demo/boademo
```

or, once the library exists in `../distribution`:

```sh
cd demo && make run
```

## Using it

* **Type a number** (1–13) and press Enter to run that demo.
* **Click** a menu item with the mouse (in a mouse-capable terminal).
* **`r`** opens a small REPL — type a line of BoaScript and see its output;
  an empty line returns to the menu.
* **`q`** quits.

When stdin is not a terminal (e.g. piped input) the demo falls back to a
plain menu with line-based number entry, so it can be scripted:

```sh
printf '1\n8\nq\n' | ./demo/boademo
```

## Demos

Arithmetic & precedence, variables & assignment, number bases, strings,
string manipulation, control flow, functions & recursion, arrays,
multi-dimensional arrays, math builtins, numerical integration
(`intgauss3`), colored output (the `color()` builtin), and
`version()`/`about()`.
