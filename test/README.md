# BoaScript test suite

Two complementary test suites, modeled on the ooyacc project's harness.

## 1. Embedded assertion suite — `testboascript`

`testboascript.cpp` holds a table of `{ script, expected, comparator }`
cases run directly against `Boascript::Calc`. Comparators are exact-string,
substring, and numeric (parse-and-compare) matches. Good for precise,
self-contained assertions.

## 2. Golden-file suite — `driver` + `cases/`

Modeled on the ooyacc suite: a generic `driver` runs a BoaScript program
and prints the interpreter's output; `run_tests.sh` diffs that output
against a golden file.

```
test/
  driver.cpp        generic main(): reads a program (file arg or stdin),
                    runs Boascript::Calc, writes output to stdout
  run_tests.sh      the harness (run -> diff), one case at a time
  cases/
    <name>.boa      a BoaScript program
    <name>.expected exact expected stdout
```

## Running

```sh
make            # from the repo root: build the library (needs ooyacc)
make test       # build + run both suites
```

or, from this directory once the library exists in `../distribution`:

```sh
make check      # both suites
bash run_tests.sh   # golden-file suite only
```

## Cases and what they cover

| Case         | Feature area                                                        |
|--------------|---------------------------------------------------------------------|
| `arithmetic` | `+ - * / % ^`, unary minus, precedence, parentheses                 |
| `variables`  | single-letter variables, compound assignment, `++`/`--`             |
| `named`      | multi-character variables, interop with single-letter, loops        |
| `strings`    | concatenation, `strlen`, `substr`, `replace`, `substitute`, `tostr`/`tonum` |
| `control`    | `if`/`else`, dangling-else, `while`                                  |
| `comparison` | relational, `== !=`, `&& || !`, bitwise `& |`, shifts `<< >>`       |
| `math`       | `sqrt cbrt pow exp log log10 abs floor ceil min max sin cos`         |
| `ternary`    | `?:` operator and the `ifn()` compatibility builtin                 |
| `precision`  | `setprec`/`getprec`, `pi()`, output formatting                      |
| `errors`     | a malformed program reports a syntax error instead of crashing      |

## Adding a golden-file case

Drop `<name>.boa` and `<name>.expected` into `cases/`. The harness picks it
up automatically. To (re)generate the golden output for a case after
confirming the program is correct:

```sh
./driver cases/<name>.boa > cases/<name>.expected
```

Because `println` appends a newline and `print` does not, and numbers use
the current precision (default 7 fixed decimals), golden files must match
byte-for-byte — regenerate them with the driver rather than editing by hand.
