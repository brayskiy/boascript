/**********************************************************************

Filename    :   boa.cpp
Content     :   Interactive BoaScript interpreter (REPL).

A Python-style read-eval-print loop over a persistent Boascript session:
variables, functions, and arrays defined on one line remain in effect on
the next. A bare expression is echoed (its value is printed); statements
run silently. Unbalanced braces/parens continue on a "..." prompt.

A small built-in line editor provides history (Up/Down arrows recall
previous/next inputs) and in-line editing (Left/Right, Home/End,
backspace/delete) with no external dependency. It works on any POSIX
terminal (Linux, macOS) and on the native Windows console (Windows 10+,
which supports virtual-terminal sequences).

Type exit() or press Ctrl-D (Ctrl-Z on Windows) / EOF to leave.

**********************************************************************/

#include <BTypes.h>
#include <Boascript.tab.h>

#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <csignal>
#include <iostream>
#include <string>
#include <vector>

#if defined(_WIN32)
#  include <windows.h>
#  include <io.h>
#else
#  include <unistd.h>
#  include <termios.h>
#endif

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
//
// The editing/history logic is platform-independent; only raw-mode setup and
// the mapping of raw input to Key events differ between POSIX and Windows.
// ----------------------------------------------------------------------------

enum KeyType
{
    K_CHAR, K_ENTER, K_BACKSPACE, K_DELETE,
    K_LEFT, K_RIGHT, K_UP, K_DOWN, K_HOME, K_END,
    K_CANCEL,   // Ctrl-C: discard the current line
    K_EOF,      // Ctrl-D / Ctrl-Z: end of input on an empty line
    K_ABORT,    // read error: stop unconditionally
    K_IGNORE
};

struct Key
{
    KeyType type;
    char    ch;
    Key(KeyType t = K_IGNORE, char c = 0) : type(t), ch(c) {}
};


class LineReader
{
public:
    LineReader() : m_raw(false)
    {
        m_tty = terminalStdin() && terminalStdout();
        s_self = this;
        std::signal(SIGINT,  onSignal);
        std::signal(SIGTERM, onSignal);
        std::atexit(atexitRestore);
    }

    ~LineReader() { leaveRaw(); }

    // Read one line. Returns false on end-of-input.
    bool read(const std::string& prompt, std::string& out)
    {
        if (!m_tty)
        {
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

    static void writeOut(const std::string& s)
    {
        std::fwrite(s.data(), 1, s.size(), stdout);
        std::fflush(stdout);
    }

    // Repaint the prompt + buffer and place the cursor at position `cur`.
    static void redraw(const std::string& prompt, const std::string& buf, size_t cur)
    {
        std::string s = "\r" + prompt + buf + "\033[K";      // line + clear tail
        size_t col = prompt.size() + cur;                    // target column (0-based)
        s += "\r";
        if (col > 0) s += "\033[" + std::to_string(col) + "C";
        writeOut(s);
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
            Key k = nextKey();
            switch (k.type)
            {
            case K_ABORT:
                leaveRaw();
                return false;

            case K_EOF:
                if (buf.empty()) { leaveRaw(); return false; }
                break;                                    // non-empty: ignore

            case K_ENTER:
                leaveRaw();
                std::printf("\n");
                std::fflush(stdout);
                out = buf;
                if (!buf.empty() && (m_history.empty() || m_history.back() != buf))
                {
                    m_history.push_back(buf);
                }
                return true;

            case K_BACKSPACE: if (cur > 0) { buf.erase(cur - 1, 1); --cur; } break;
            case K_DELETE:    if (cur < buf.size()) buf.erase(cur, 1);       break;
            case K_LEFT:      if (cur > 0) --cur;                            break;
            case K_RIGHT:     if (cur < buf.size()) ++cur;                   break;
            case K_HOME:      cur = 0;                                       break;
            case K_END:       cur = buf.size();                             break;

            case K_UP:
                if (hpos > 0)
                {
                    if (hpos == m_history.size()) saved = buf;
                    --hpos;
                    buf = m_history[hpos];
                    cur = buf.size();
                }
                break;

            case K_DOWN:
                if (hpos < m_history.size())
                {
                    ++hpos;
                    buf = (hpos == m_history.size()) ? saved : m_history[hpos];
                    cur = buf.size();
                }
                break;

            case K_CANCEL:
                buf.clear(); cur = 0; hpos = m_history.size();
                leaveRaw(); std::printf("^C\n"); std::fflush(stdout); enterRaw();
                break;

            case K_CHAR:
                buf.insert(buf.begin() + cur, k.ch);
                ++cur;
                break;

            default:
                break;
            }
            redraw(prompt, buf, cur);
        }
    }

// -------------------------- platform-specific ------------------------------
#if defined(_WIN32)

    DWORD m_inMode;
    DWORD m_outMode;

    static bool terminalStdin()  { return _isatty(_fileno(stdin))  != 0; }
    static bool terminalStdout() { return _isatty(_fileno(stdout)) != 0; }

    void enterRaw()
    {
        if (m_raw) return;
        HANDLE hIn  = GetStdHandle(STD_INPUT_HANDLE);
        HANDLE hOut = GetStdHandle(STD_OUTPUT_HANDLE);
        GetConsoleMode(hIn,  &m_inMode);
        GetConsoleMode(hOut, &m_outMode);
        // Raw input: read keys individually, no echo, deliver Ctrl-C as input.
        SetConsoleMode(hIn, m_inMode &
                       ~(ENABLE_LINE_INPUT | ENABLE_ECHO_INPUT | ENABLE_PROCESSED_INPUT));
        // Let the console interpret the ANSI escapes redraw() emits.
        SetConsoleMode(hOut, m_outMode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
        m_raw = true;
    }

    void leaveRaw()
    {
        if (!m_raw) return;
        SetConsoleMode(GetStdHandle(STD_INPUT_HANDLE),  m_inMode);
        SetConsoleMode(GetStdHandle(STD_OUTPUT_HANDLE), m_outMode);
        m_raw = false;
    }

    Key nextKey()
    {
        HANDLE       hIn = GetStdHandle(STD_INPUT_HANDLE);
        INPUT_RECORD rec;
        DWORD        n;
        for (;;)
        {
            if (!ReadConsoleInput(hIn, &rec, 1, &n) || n == 0) return Key(K_ABORT);
            if (rec.EventType != KEY_EVENT || !rec.Event.KeyEvent.bKeyDown) continue;

            WORD vk = rec.Event.KeyEvent.wVirtualKeyCode;
            char ch = rec.Event.KeyEvent.uChar.AsciiChar;

            switch (vk)
            {
            case VK_RETURN: return Key(K_ENTER);
            case VK_BACK:   return Key(K_BACKSPACE);
            case VK_DELETE: return Key(K_DELETE);
            case VK_LEFT:   return Key(K_LEFT);
            case VK_RIGHT:  return Key(K_RIGHT);
            case VK_UP:     return Key(K_UP);
            case VK_DOWN:   return Key(K_DOWN);
            case VK_HOME:   return Key(K_HOME);
            case VK_END:    return Key(K_END);
            default:        break;
            }
            if (ch == 3)                       return Key(K_CANCEL);   // Ctrl-C
            if (ch == 26 || ch == 4)           return Key(K_EOF);      // Ctrl-Z / Ctrl-D
            if (ch >= 32 && (unsigned char)ch < 127) return Key(K_CHAR, ch);
            // ignore other control / dead keys
        }
    }

#else  // POSIX (Linux, macOS)

    struct termios m_orig;

    static bool terminalStdin()  { return isatty(STDIN_FILENO); }
    static bool terminalStdout() { return isatty(STDOUT_FILENO); }

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

    Key nextKey()
    {
        int c = getByte();
        if (c < 0)                return Key(K_ABORT);
        if (c == '\r' || c == '\n') return Key(K_ENTER);
        if (c == 4)               return Key(K_EOF);        // Ctrl-D
        if (c == 3)               return Key(K_CANCEL);     // Ctrl-C
        if (c == 127 || c == 8)   return Key(K_BACKSPACE);
        if (c == 1)               return Key(K_HOME);       // Ctrl-A
        if (c == 5)               return Key(K_END);        // Ctrl-E
        if (c == '\033')
        {
            int a = getByte();
            if (a != '[' && a != 'O') return Key(K_IGNORE);
            int b = getByte();
            switch (b)
            {
            case 'A': return Key(K_UP);
            case 'B': return Key(K_DOWN);
            case 'C': return Key(K_RIGHT);
            case 'D': return Key(K_LEFT);
            case 'H': return Key(K_HOME);
            case 'F': return Key(K_END);
            case '3': return (getByte() == '~') ? Key(K_DELETE) : Key(K_IGNORE);
            default:  return Key(K_IGNORE);
            }
        }
        if (c >= 32 && c < 127) return Key(K_CHAR, (char)c);
        return Key(K_IGNORE);
    }

#endif
};

LineReader* LineReader::s_self = 0;


int main()
{
#if defined(_WIN32)
    const char* os     = "windows";
    const char* eofkey = "Ctrl-Z";
#elif defined(__APPLE__)
    const char* os     = "darwin";
    const char* eofkey = "Ctrl-D";
#elif defined(__CYGWIN__)
    const char* os     = "cygwin";
    const char* eofkey = "Ctrl-D";
#else
    const char* os     = "linux";
    const char* eofkey = "Ctrl-D";
#endif

    std::printf("BoaScript %s on %s\n", BOASCRIPT_VERSION, os);
    std::printf("%s\n", BOASCRIPT_DESCRIPTION);
    std::printf("Type \"exit()\" or %s (i.e. EOF) to exit.\n", eofkey);

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
            break;                          // EOF
        }

        // Exit commands are handled here so BoaScript's own 'exit' keyword
        // (which would terminate the process) never reaches the interpreter.
        if (buffer.empty())
        {
            std::string t0 = trim(line);
            if (t0.empty()) continue;
            if (t0 == "exit" || t0 == "quit")
            {
                std::printf("Use exit() or %s (i.e. EOF) to exit\n", eofkey);
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
