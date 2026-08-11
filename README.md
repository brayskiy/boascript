# BoaScript

BoaScript is a light, easy scripting language — and a powerful calculator —
with C-style syntax. It is embedded as a C++ library: you hand it a script
string and get the output back.

![Boascript](image/boascript-web.png)

## Repository Structure

* [APP](src) — BoaScript implementation (the `ooyacc` grammar `Boascript.y`).
* [EXTRAS](extras) — supporting classes (types, tokenizer, date/time).
* [TEST](test) — test suites, golden-file cases, and coverage tooling.
* [DOC](doc) — the [language reference](doc/reference.md).

## Getting Started

```cpp
#include <BTypes.h>
#include <Boascript.tab.h>
using namespace BoriSoft;

Boascript bs;
std::string& res = bs.run("x = 0; b = 2; s = (b - x) / 500; "
                           "while (x <= b) { y = sin(x); x += s; } println x;");
std::cout << res << std::endl;   // 2.0000000
```

`run` also accepts and returns a `Column` (vector of strings) for batch use.

## Language Features

* **Values**: numbers (decimal, scientific, and `0x`/`0b`/`0o` integer
  literals) and strings.
* **Variables**: single-letter and multi-character names, `=`, and compound
  assignment (`+= -= *= /= %=`, `++`, `--`).
* **Operators**: arithmetic (`+ - * / % ^`), comparison, logical
  (`&& || !`), bitwise (`& |`), and shifts (`<< >>`), with C-style
  precedence; the ternary `?:`.
* **Control flow**: `if`/`else`, `while`, C-style `for`, and
  `case`/`when`/`else`.
* **Functions**: `func name(params) { … return e; }` with recursion and
  isolated local scope; plus the `intgauss3(f, a, b)` Gauss-Legendre
  integration builtin.
* **Math builtins**: `sqrt cbrt pow hypot exp log log10 abs floor ceil`,
  trigonometric / inverse / hyperbolic functions, `min max`, `pi()`, and
  more.
* **Strings**: concatenation with `+` (numbers stringify), `strlen`/`len`,
  `substr`, `substitute`, `upper`, `lower`, `reverse`, `find`, `repeat`,
  `charat`, `tostr`, `tonum`.
* **Arrays**: `[..]` literals, `a[i]` get/set, and `len`/`sum`/`avg`/`max`/
  `min`/`prod` reductions — including **multi-dimensional** (nested) arrays
  with `m[i][j][k]` index chains.
* **Introspection**: `version()` returns the release version (`YY.WW.BB`),
  and `about()` / `description()` return the application description.

See the [language reference](doc/reference.md) for full details and examples.

## Dependencies

Building the BoaScript library requires the
[ooyacc](https://github.com/brayskiy/ooyacc) parser generator (install it so
`ooyacc` is on your `PATH` or at `$HOME/bin/ooyacc`).

## Building

```sh
make all          # generate the parser, build distribution/libBoascript.a
```

## Testing

```sh
make test         # build the library and run every test suite
```

Three complementary suites live under [test/](test):

* **`testboascript`** — an embedded table of assertion cases.
* **`unittests`** — focused unit tests over the API and every construct.
* **golden-file cases** — `driver` runs `cases/*.boa` and diffs the output
  against `cases/*.expected`.

`make coverage` (in `test/`) reports line and branch coverage via `gcov`.
See [test/README.md](test/README.md) for details.

## Releases and versioning

The release version has the form `YY.WW.BB` (two-digit year, ISO week,
patch). It is promoted automatically each time `develop` is merged into
`master`: the [release workflow](.github/workflows/release.yml) computes the
next patch for the current year+week (restarting at `01` when the week
rolls over) and creates a `vYY.WW.BB` git tag and GitHub Release. The build
bakes the latest tag into the library, where `version()` exposes it.

## Applications using BoaScript

[Boascript](https://play.google.com/store/apps/details?id=boris.boascript)

[Calclab](https://play.google.com/store/apps/details?id=boris.calclab.free)
is a graphing calculator (also graphics / graphic calculator) — a class of
scientific calculators capable of plotting graphs (Cartesian two- and
three-dimensional, Polar, and Parametric) and performing calculations using
the BoaScript scripting language. BoaScript has a C-style syntax and allows
calculations across a wide range — from arithmetic and trigonometric
calculations to complex numerical solutions.
