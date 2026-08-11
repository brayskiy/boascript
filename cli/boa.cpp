/**********************************************************************

Filename    :   boa.cpp
Content     :   Interactive BoaScript interpreter (REPL).

A Python-style read-eval-print loop over a persistent Boascript session:
variables, functions, and arrays defined on one line remain in effect on
the next. A bare expression is echoed (its value is printed); statements
run silently. Unbalanced braces/parens continue on a "..." prompt.

A small built-in line editor provides history (Up/Down arrows recall
previous/next inputs) and in-line editing (Left/Right, Home/End,
backspace/delete) without any external dependency. Type exit() or press
Ctrl-D (EOF) to leave.

**********************************************************************/

#include <BTypes.h>
#include <Boascript.tab.h>

#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

#include <unistd.h>
#include <termios.h>
#include <csignal>

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


// ----------------------------------------------------------------------------
// Line editor with command history
// ----------------------------------------------------------------------------

class LineReader
{
public:
    LineReader()
        : m_tty(isatty(STDIN_FILENO) && isatty(STDOUT_FILENO)), m_raw(false)
    {
        s_self = this;
        std::signal(SIGINT,  onSignal);
        std::signal(SIGTERM, onSignal);
        std::atexit(atexitRestore);
    }

    ~LineReader() { leaveRaw(); }

    // Read one line. Returns false on end-of-input (Ctrl-D on an empty line).
    bool read(const std::string& prompt, std::string& out)
    {
        if (!m_tty)
        {
            // Not a terminal (piped input): plain line input, no editing.
            std::string line;
            if (!std::getline(std::cin, line)) return false;
            out = line;
            return true;
        }
        return readInteractive(prompt, out);
    }

private:
    bool                     m_tty;
    bool                     m_raw;
    struct termios           m_orig;
    std::vector<std::string> m_history;

    static LineReader*       s_self;

    static void atexitRestore() { if (s_self) s_self->leaveRaw(); }
    static void onSignal(int)
    {
        if (s_self) s_self->leaveRaw();
        std::printf("\n");
        std::fflush(stdout);
        std::_Exit(0);
    }

    void enterRaw()
    {
        if (m_raw) return;
        tcgetattr(STDIN_FILENO, &m_orig);
        struct termios t = m_orig;
        t.c_lflag &= ~(ICANON | ECHO | ISIG | IEXTEN);
        t.c_iflag &= ~(IXON | ICRNL);
        t.c_cc[VMIN]  = 1;
        t.c_cc[VTIME] = 0;
        tcsetattr(STDIN_FILENO, TCSANOW, &t);
        m_raw = true;
    }

    void leaveRaw()
    {
        if (!m_raw) return;
        tcsetattr(STDIN_FILENO, TCSANOW, &m_orig);
        m_raw = false;
    }

    static int getByte()
    {
        unsigned char c;
        ssize_t n = ::read(STDIN_FILENO, &c, 1);
        return (n == 1) ? (int)c : -1;
    }

    // Repaint the prompt + buffer and place the cursor at position `cur`.
    static void redraw(const std::string& prompt, const std::string& buf, size_t cur)
    {
        std::string s = "\r" + prompt + buf + "\033[K";      // line + clear tail
        size_t col = prompt.size() + cur;                    // target column (0-based)
        s += "\r";
        if (col > 0) s += "\033[" + std::to_string(col) + "C";
        (void)!::write(STDOUT_FILENO, s.data(), s.size());
    }

    bool readInteractive(const std::string& prompt, std::string& out)
    {
        enterRaw();

        std::string buf;
        size_t      cur  = 0;
        size_t      hpos = m_history.size();   // == size() means "new line"
        std::string saved;                     // in-progress line while browsing

        redraw(prompt, buf, cur);

        for (;;)
        {
            int c = getByte();
            if (c < 0) { leaveRaw(); return false; }

            if (c == '\r' || c == '\n')
            {
                leaveRaw();
                std::printf("\n");
                std::fflush(stdout);
                out = buf;
                if (!buf.empty() && (m_history.empty() || m_history.back() != buf))
                {
                    m_history.push_back(buf);
                }
                return true;
            }
            if (c == 4)                                   // Ctrl-D
            {
                if (buf.empty()) { leaveRaw(); return false; }
                if (cur < buf.size()) buf.erase(cur, 1);  // else: delete at cursor
            }
            else if (c == 3)                              // Ctrl-C: cancel the line
            {
                buf.clear(); cur = 0; hpos = m_history.size();
                leaveRaw(); std::printf("^C\n"); std::fflush(stdout); enterRaw();
            }
            else if (c == 127 || c == 8)                  // backspace
            {
                if (cur > 0) { buf.erase(cur - 1, 1); --cur; }
            }
            else if (c == 1) { cur = 0; }                 // Ctrl-A: home
            else if (c == 5) { cur = buf.size(); }        // Ctrl-E: end
            else if (c == '\033')                         // escape sequence
            {
                int a = getByte();
                if (a != '[' && a != 'O') continue;
                int b = getByte();
                if (b == 'A')                             // Up: previous history
                {
                    if (hpos > 0)
                    {
                        if (hpos == m_history.size()) saved = buf;
                        --hpos;
                        buf = m_history[hpos];
                        cur = buf.size();
                    }
                }
                else if (b == 'B')                        // Down: next history
                {
                    if (hpos < m_history.size())
                    {
                        ++hpos;
                        buf = (hpos == m_history.size()) ? saved : m_history[hpos];
                        cur = buf.size();
                    }
                }
                else if (b == 'C') { if (cur < buf.size()) ++cur; }   // Right
                else if (b == 'D') { if (cur > 0) --cur; }            // Left
                else if (b == 'H') { cur = 0; }                       // Home
                else if (b == 'F') { cur = buf.size(); }              // End
                else if (b == '3')                                   // Delete
                {
                    if (getByte() == '~' && cur < buf.size()) buf.erase(cur, 1);
                }
            }
            else if (c >= 32 && c < 127)                  // printable
            {
                buf.insert(buf.begin() + cur, (char)c);
                ++cur;
            }

            redraw(prompt, buf, cur);
        }
    }
};

LineReader* LineReader::s_self = 0;


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

    LineReader reader;
    std::string buffer;
    std::string line;
    for (;;)
    {
        if (!reader.read(buffer.empty() ? ">>> " : "... ", line))
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
