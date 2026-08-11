/**********************************************************************

Filename    :   boa.cpp
Content     :   Interactive BoaScript interpreter (REPL).

A Python-style read-eval-print loop over a persistent Boascript session:
variables, functions, and arrays defined on one line remain in effect on
the next. A bare expression is echoed (its value is printed); statements
run silently. Unbalanced braces/parens continue on a "..." prompt. Type
exit() or press Ctrl-D (EOF) to leave.

**********************************************************************/

#include <BTypes.h>
#include <Boascript.tab.h>

#include <cctype>
#include <cstdio>
#include <iostream>
#include <string>

using namespace BoriSoft;


static std::string trim(const std::string& s)
{
    size_t a = s.find_first_not_of(" \t\r\n");
    if (a == std::string::npos) return "";
    size_t b = s.find_last_not_of(" \t\r\n");
    return s.substr(a, b - a + 1);
}


static std::string firstWord(const std::string& s)
{
    size_t i = 0;
    while (i < s.size() && std::isspace((unsigned char)s[i])) ++i;
    size_t j = i;
    while (j < s.size() && (std::isalnum((unsigned char)s[j]) || s[j] == '_')) ++j;
    return s.substr(i, j - i);
}


// Net count of unclosed { ( [ (ignoring string bodies and // comments), so
// the REPL knows when to keep reading a multi-line construct.
static int openDelims(const std::string& s)
{
    int  n     = 0;
    bool inStr = false;
    for (size_t i = 0; i < s.size(); ++i)
    {
        char c = s[i];
        if (inStr) { if (c == '"') inStr = false; continue; }
        if (c == '"') { inStr = true; continue; }
        if (c == '/' && i + 1 < s.size() && s[i + 1] == '/')
        {
            while (i < s.size() && s[i] != '\n') ++i;
            continue;
        }
        if (c == '{' || c == '(' || c == '[') ++n;
        else if (c == '}' || c == ')' || c == ']') --n;
    }
    return n;
}


// Is there a top-level assignment operator (outside string literals)? Used to
// tell an assignment statement from a bare expression.
static bool hasAssignment(const std::string& s)
{
    bool inStr = false;
    for (size_t i = 0; i < s.size(); ++i)
    {
        char c = s[i];
        if (inStr) { if (c == '"') inStr = false; continue; }
        if (c == '"') { inStr = true; continue; }
        if (c == '~') return true;
        if ((c == '+' || c == '-' || c == '*' || c == '/' || c == '%') &&
            i + 1 < s.size() && s[i + 1] == '=')
        {
            return true;   // compound assignment
        }
        if (c == '=')
        {
            char prev = i ? s[i - 1] : 0;
            char next = (i + 1 < s.size()) ? s[i + 1] : 0;
            if (next == '=') continue;                       // ==
            if (prev == '<' || prev == '>' || prev == '!' ||
                prev == '=' || prev == '+' || prev == '-' ||
                prev == '*' || prev == '/' || prev == '%')
            {
                continue;                                    // <= >= != or compound
            }
            return true;                                     // plain assignment
        }
    }
    return false;
}


// A statement runs silently; anything else is treated as an expression whose
// value is echoed.
static bool isStatement(const std::string& t)
{
    if (t.empty()) return true;
    char last = t[t.size() - 1];
    if (last == ';' || last == '}') return true;
    std::string w = firstWord(t);
    if (w == "print" || w == "println" || w == "if" || w == "while" ||
        w == "for" || w == "func" || w == "return" || w == "case" ||
        w == "when" || w == "exit")
    {
        return true;
    }
    return hasAssignment(t);
}


int main()
{
#if defined(BOS_WINDOWS)
    const char* os = "windows";
#elif defined(BOS_OSX)
    const char* os = "darwin";
#elif defined(BOS_CYGWIN)
    const char* os = "cygwin";
#else
    const char* os = "linux";
#endif

    std::printf("BoaScript %s on %s\n", BOASCRIPT_VERSION, os);
    std::printf("%s\n", BOASCRIPT_DESCRIPTION);
    std::printf("Type \"exit()\" or Ctrl-D (i.e. EOF) to exit.\n");

    Boascript bs;
    bs.beginSession();

    std::string buffer;
    std::string line;
    for (;;)
    {
        std::printf("%s", buffer.empty() ? ">>> " : "... ");
        std::fflush(stdout);

        if (!std::getline(std::cin, line))
        {
            std::printf("\n");
            break;                          // Ctrl-D / EOF
        }

        // Exit commands are handled here so BoaScript's own 'exit' keyword
        // (which would terminate the process) never reaches the interpreter.
        if (buffer.empty())
        {
            std::string t0 = trim(line);
            if (t0.empty()) continue;
            if (t0 == "exit" || t0 == "quit")
            {
                std::printf("Use exit() or Ctrl-D (i.e. EOF) to exit\n");
                continue;
            }
            if (t0 == "exit()" || t0 == "quit()" ||
                t0 == "exit();" || t0 == "quit();")
            {
                break;
            }
        }

        buffer += line;
        buffer += "\n";
        if (openDelims(buffer) > 0) continue;   // unbalanced: read more

        std::string t = trim(buffer);
        buffer.clear();
        if (t.empty()) continue;

        std::string prog;
        if (isStatement(t))
        {
            prog = t;
            char last = prog[prog.size() - 1];
            if (last != ';' && last != '}') prog += ";";
        }
        else
        {
            prog = "println (" + t + ");";      // echo the expression's value
        }

        std::string out = bs.runLine(prog);
        std::printf("%s", out.c_str());
        if (!out.empty() && out[out.size() - 1] != '\n') std::printf("\n");
        std::fflush(stdout);
    }

    bs.endSession();
    return 0;
}
