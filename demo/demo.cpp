/**********************************************************************

Filename    :   demo.cpp
Content     :   Interactive CLI demo for the BoaScript library.

A menu of feature demos. Pick one by typing its number and pressing
Enter, or -- in a mouse-capable terminal -- by clicking the menu item.
Each demo prints the BoaScript source it runs and the output produced
by Boascript::run().

**********************************************************************/

#include <BTypes.h>
#include <Boascript.tab.h>

#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>
#include <vector>

#include <unistd.h>
#include <termios.h>
#include <csignal>

using namespace BoriSoft;


// ----------------------------------------------------------------------------
// Demo catalogue
// ----------------------------------------------------------------------------

struct Demo
{
    const char* title;
    const char* desc;
    const char* script;
};

static const Demo DEMOS[] =
{
    { "Arithmetic & precedence",
      "operators + - * / % ^, unary minus, parentheses",
      "println 2 + 3 * 4;\n"
      "println (2 + 3) * 4;\n"
      "println 2 ^ 10;\n"
      "println 17 % 5;\n"
      "println -5 + 8;" },

    { "Variables & assignment",
      "named variables, compound assignment, ++/--",
      "total = 10;\n"
      "total += 5;\n"
      "total *= 2;\n"
      "println total;\n"
      "count = 0; ++count; ++count;\n"
      "println count;" },

    { "Number bases",
      "hexadecimal, binary, and octal integer literals",
      "println 0xFF;\n"
      "println 0b1010;\n"
      "println 0o17;\n"
      "println 0xFF + 0b1 + 0o1;" },

    { "Strings",
      "concatenation and string builtins",
      "s = \"Hello, \" + \"World\";\n"
      "println s;\n"
      "println upper(s);\n"
      "println reverse(s);\n"
      "println len(s);\n"
      "println substr(s, 7, 5);\n"
      "println \"answer = \" + 42;" },

    { "String manipulation",
      "find, repeat, charat, substitute",
      "s = \"banana\";\n"
      "println find(s, \"nan\");\n"
      "println repeat(\"ab\", 3);\n"
      "println charat(s, 0);\n"
      "println substitute(s, \"a\", \"A\");" },

    { "Control flow",
      "for, while, if/else, case/when",
      "for (i = 1; i <= 5; i += 1) { print i; print \" \"; }\n"
      "println \"\";\n"
      "n = 3;\n"
      "case (n) {\n"
      "    when 1: println \"one\";\n"
      "    when 3: println \"three\";\n"
      "    else:   println \"other\";\n"
      "}" },

    { "Functions & recursion",
      "func definitions, return, recursion, local scope",
      "func fact(n) {\n"
      "    if (n <= 1) { return 1; }\n"
      "    return n * fact(n - 1);\n"
      "}\n"
      "println fact(5);\n"
      "func max2(a, b) { if (a > b) { return a; } return b; }\n"
      "println max2(10, 7);" },

    { "Arrays",
      "literals, indexing, reductions",
      "a = [5, 2, 8, 1, 9];\n"
      "print a; println \"\";\n"
      "println sum(a);\n"
      "println avg(a);\n"
      "println max(a);\n"
      "a[0] = 100;\n"
      "print a; println \"\";" },

    { "Multi-dimensional arrays",
      "nested literals, index chains, recursive sum",
      "m = [[1, 2, 3], [4, 5, 6]];\n"
      "print m; println \"\";\n"
      "println m[1][2];\n"
      "println len(m);\n"
      "println sum(m);\n"
      "m[0][0] = 99;\n"
      "print m; println \"\";" },

    { "Math builtins",
      "sqrt, pow, hypot, trig, pi",
      "println sqrt(2);\n"
      "println pow(2, 10);\n"
      "println hypot(3, 4);\n"
      "println pi();" },

    { "Numerical integration",
      "3-point Gauss-Legendre via the intgauss3 builtin",
      "setprec(6);\n"
      "func f(x) { return x * x; }\n"
      "println intgauss3(f, 0, 1);\n"
      "func g(x) { return sin(x); }\n"
      "println intgauss3(g, 0, pi());" },

    { "Version & description",
      "the version() and about() builtins",
      "println version();\n"
      "println about();" },
};

static const int N_DEMOS = (int)(sizeof(DEMOS) / sizeof(DEMOS[0]));

// ANSI helpers.
static const char* BOLD  = "\033[1m";
static const char* DIM   = "\033[2m";
static const char* CYAN  = "\033[36m";
static const char* GREEN = "\033[32m";
static const char* RESET = "\033[0m";


// ----------------------------------------------------------------------------
// Running a demo
// ----------------------------------------------------------------------------

static void runDemo(const Demo& d)
{
    std::printf("\n%s%s=== %s ===%s\n", BOLD, CYAN, d.title, RESET);
    std::printf("%s%s%s\n\n", DIM, d.desc, RESET);

    std::printf("%s--- script ---%s\n%s\n\n", BOLD, RESET, d.script);

    Boascript bs;
    std::string out = bs.run(d.script);

    std::printf("%s--- output ---%s\n%s%s%s", BOLD, RESET, GREEN, out.c_str(), RESET);
    if (out.empty() || out[out.size() - 1] != '\n')
    {
        std::printf("\n");
    }
    std::fflush(stdout);
}


// A tiny read-eval-print loop for user-supplied scripts.
static void repl()
{
    std::printf("\n%s%sBoaScript REPL%s  -- one statement group per line; "
                "empty line returns to the menu.\n", BOLD, CYAN, RESET);
    std::string line;
    for (;;)
    {
        std::printf("%sboa>%s ", BOLD, RESET);
        std::fflush(stdout);
        if (!std::getline(std::cin, line) || line.empty())
        {
            break;
        }
        Boascript bs;
        std::string out = bs.run(line);
        std::printf("%s%s%s", GREEN, out.c_str(), RESET);
        if (out.empty() || out[out.size() - 1] != '\n')
        {
            std::printf("\n");
        }
    }
}


// ----------------------------------------------------------------------------
// Terminal handling (raw mode + SGR mouse) for the interactive menu
// ----------------------------------------------------------------------------

static struct termios g_orig;
static bool           g_raw = false;

static void leaveRaw()
{
    if (!g_raw) return;
    std::printf("\033[?1000l\033[?1006l");   // disable mouse tracking
    std::fflush(stdout);
    tcsetattr(STDIN_FILENO, TCSANOW, &g_orig);
    g_raw = false;
}

static void enterRaw()
{
    if (g_raw) return;
    tcgetattr(STDIN_FILENO, &g_orig);
    struct termios t = g_orig;
    t.c_lflag &= ~(ICANON | ECHO);
    t.c_cc[VMIN]  = 1;
    t.c_cc[VTIME] = 0;
    tcsetattr(STDIN_FILENO, TCSANOW, &t);
    std::printf("\033[?1000h\033[?1006h");   // enable click + SGR mouse tracking
    std::fflush(stdout);
    g_raw = true;
}

static void onSignal(int)
{
    leaveRaw();
    std::printf("\033[?25h\n");   // show cursor
    std::fflush(stdout);
    std::_Exit(0);
}

static int readByte()
{
    unsigned char c;
    ssize_t n = read(STDIN_FILENO, &c, 1);
    return (n == 1) ? (int)c : -1;
}


// Rows (1-based terminal lines) that each selectable entry is drawn on, so a
// mouse click can be mapped back to a selection.
struct Rows
{
    std::vector<int> demo;   // demo[i] -> row of demo i
    int              repl;
    int              quit;
    Rows() : demo(N_DEMOS, 0), repl(0), quit(0) {}
};

static void render(const std::string& numbuf, Rows& rows)
{
    std::string buf = "\033[H\033[2J";   // home + clear screen
    int row = 1;
    // Append one line and advance the row counter.
    struct Add {
        std::string& b; int& r;
        void operator()(const std::string& s) { b += s; b += "\r\n"; ++r; }
    } add { buf, row };

    add(std::string(BOLD) + CYAN + "  BoaScript feature demo" + RESET);
    add(std::string(DIM) + "  Type a number and Enter, or click an item. "
        "'r' = REPL, 'q' = quit." + RESET);
    add("");

    for (int i = 0; i < N_DEMOS; ++i)
    {
        char num[8];
        std::snprintf(num, sizeof num, "%2d", i + 1);
        rows.demo[i] = row;
        add(std::string("   ") + BOLD + num + RESET + ". " + DEMOS[i].title +
            "  " + DIM + DEMOS[i].desc + RESET);
    }

    add("");
    rows.repl = row;
    add(std::string("    ") + BOLD + "r" + RESET + ". Run your own script (REPL)");
    rows.quit = row;
    add(std::string("    ") + BOLD + "q" + RESET + ". Quit");
    add("");
    buf += std::string("  Select: ") + numbuf;

    (void)write(STDOUT_FILENO, buf.data(), buf.size());
}


// Selection results.
enum { SEL_QUIT = -1, SEL_REPL = -2 };

// Read input until the user makes a selection; returns a demo index in
// [0, N_DEMOS), or SEL_QUIT / SEL_REPL.
static int menuSelect()
{
    std::string numbuf;
    Rows rows;
    for (;;)
    {
        render(numbuf, rows);

        int c = readByte();
        if (c < 0 || c == 'q' || c == 'Q')          return SEL_QUIT;
        if (c == 'r' || c == 'R')                    return SEL_REPL;

        if (c == '\r' || c == '\n')
        {
            if (!numbuf.empty())
            {
                int v = std::atoi(numbuf.c_str());
                numbuf.clear();
                if (v >= 1 && v <= N_DEMOS) return v - 1;
            }
            continue;
        }
        if (c == 127 || c == 8)                       // backspace
        {
            if (!numbuf.empty()) numbuf.erase(numbuf.size() - 1);
            continue;
        }
        if (c >= '0' && c <= '9')
        {
            if (numbuf.size() < 2) numbuf.push_back((char)c);
            continue;
        }
        if (c == '\033')                              // escape sequence
        {
            if (readByte() != '[') continue;
            int c2 = readByte();
            if (c2 != '<') continue;                  // only SGR mouse here

            std::string seq;
            int fin = 0;
            for (;;)
            {
                int ch = readByte();
                if (ch < 0) break;
                if (ch == 'M' || ch == 'm') { fin = ch; break; }
                seq.push_back((char)ch);
            }
            int b = 0, x = 0, y = 0;
            if (std::sscanf(seq.c_str(), "%d;%d;%d", &b, &x, &y) == 3 &&
                fin == 'M' && (b & 0x63) == 0)        // left button press, no drag/wheel
            {
                for (int i = 0; i < N_DEMOS; ++i)
                {
                    if (rows.demo[i] == y) return i;
                }
                if (y == rows.repl) return SEL_REPL;
                if (y == rows.quit) return SEL_QUIT;
            }
            continue;
        }
    }
}


static void waitForKey()
{
    std::printf("\n%sPress Enter to return to the menu...%s", DIM, RESET);
    std::fflush(stdout);
    std::string line;
    std::getline(std::cin, line);
}


static void interactive()
{
    std::signal(SIGINT, onSignal);
    std::signal(SIGTERM, onSignal);
    std::atexit(leaveRaw);

    for (;;)
    {
        enterRaw();
        int sel = menuSelect();
        leaveRaw();

        if (sel == SEL_QUIT) break;

        std::printf("\033[H\033[2J");
        if (sel == SEL_REPL) { repl(); }
        else                 { runDemo(DEMOS[sel]); waitForKey(); }
    }

    std::printf("\033[H\033[2J");
    std::fflush(stdout);
}


// Non-interactive fallback (piped stdin / no TTY): plain menu, line input.
static void batch()
{
    for (;;)
    {
        std::printf("\nBoaScript feature demo\n");
        for (int i = 0; i < N_DEMOS; ++i)
        {
            std::printf("  %2d. %s\n", i + 1, DEMOS[i].title);
        }
        std::printf("   r. Run your own script (REPL)\n");
        std::printf("   q. Quit\n");
        std::printf("Select: ");
        std::fflush(stdout);

        std::string line;
        if (!std::getline(std::cin, line)) break;
        if (line == "q" || line == "Q" || line.empty()) break;
        if (line == "r" || line == "R") { repl(); continue; }

        int v = std::atoi(line.c_str());
        if (v >= 1 && v <= N_DEMOS) runDemo(DEMOS[v - 1]);
        else std::printf("  (no such item: %s)\n", line.c_str());
    }
}


int main()
{
    if (isatty(STDIN_FILENO) && isatty(STDOUT_FILENO))
    {
        interactive();
    }
    else
    {
        batch();
    }
    return 0;
}
