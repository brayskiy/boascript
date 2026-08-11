/*
 * Focused unit tests for BoaScript, exercising the interpreter API and
 * every language construct, builtin, operator, and error path. Together
 * with testboascript.cpp and the golden-file cases these drive the
 * coverage measured by coverage.sh.
 *
 * Self-contained: no framework. Each check compares Boascript::Calc output
 * (exact string, substring, or numeric) and tallies pass/fail; main returns
 * non-zero if anything fails.
 */

#include <BTypes.h>
#include <Boascript.tab.h>
#include <Tokenizer.h>

#include <cmath>
#include <iostream>
#include <sstream>
#include <string>

using namespace BoriSoft;

static int g_pass = 0;
static int g_fail = 0;

static std::string run(const std::string& src)
{
    Boascript bs;
    return bs.Calc(src);
}

// Exact string match of the whole output.
static void eqStr(const char* what, const std::string& src, const std::string& expect)
{
    std::string got = run(src);
    if (got == expect) { ++g_pass; return; }
    ++g_fail;
    std::cout << "FAIL [" << what << "]: got [" << got
              << "] expected [" << expect << "]\n";
}

// Substring match (for error messages etc.).
static void hasStr(const char* what, const std::string& src, const std::string& needle)
{
    std::string got = run(src);
    if (got.find(needle) != std::string::npos) { ++g_pass; return; }
    ++g_fail;
    std::cout << "FAIL [" << what << "]: [" << got
              << "] does not contain [" << needle << "]\n";
}

// Numeric match within a tolerance (parses the leading number of the output).
static void eqNum(const char* what, const std::string& src, double expect, double tol = 1e-6)
{
    std::string got = run(src);
    double d = 0.0;
    util::Tokenizer::string2double(got, d);
    if (std::fabs(d - expect) <= tol) { ++g_pass; return; }
    ++g_fail;
    std::cout << "FAIL [" << what << "]: got " << d
              << " expected " << expect << " (raw [" << got << "])\n";
}

// Just execute a program (for nondeterministic output like rand/date):
// the check is only that it runs without throwing.
static void runOnly(const char* what, const std::string& src)
{
    try { run(src); ++g_pass; }
    catch (...) { ++g_fail; std::cout << "FAIL [" << what << "]: threw\n"; }
}


static void testArithmetic()
{
    eqNum("add",        "println 2 + 3;",        5);
    eqNum("sub",        "println 10 - 3;",       7);
    eqNum("mul",        "println 4 * 5;",        20);
    eqNum("div",        "println 7 / 2;",        3.5);
    eqNum("div0",       "println 5 / 0;",        0);      // zero-divisor branch
    eqNum("mod",        "println 17 % 5;",       2);
    eqNum("mod0",       "println 5 % 0;",        0);      // zero-divisor branch
    eqNum("pow_caret",  "println 2 ^ 10;",       1024);
    eqNum("uminus",     "println -8;",           -8);
    eqNum("paren",      "println (2 + 3) * 4;",  20);
    eqNum("preinc",     "a = 1; ++a; println a;", 2);
    eqNum("predec",     "a = 5; --a; println a;", 4);
}


static void testAssignOps()
{
    eqNum("plus_assign",  "a = 5; a += 3; println a;", 8);
    eqNum("minus_assign", "a = 5; a -= 2; println a;", 3);
    eqNum("mul_assign",   "a = 5; a *= 4; println a;", 20);
    eqNum("div_assign",   "a = 8; a /= 2; println a;", 4);
    eqNum("div_assign0",  "a = 8; a /= 0; println a;", 0);   // zero branch
    eqNum("mod_assign",   "a = 17; a %= 5; println a;", 2);
    eqNum("mod_assign0",  "a = 17; a %= 0; println a;", 0);  // zero branch
}


static void testCompareNumeric()
{
    eqNum("gt1", "println 5 > 3;", 1);
    eqNum("gt0", "println 3 > 5;", 0);
    eqNum("lt1", "println 3 < 5;", 1);
    eqNum("lt0", "println 5 < 3;", 0);
    eqNum("ge1", "println 5 >= 5;", 1);
    eqNum("ge0", "println 4 >= 5;", 0);
    eqNum("le1", "println 4 <= 4;", 1);
    eqNum("le0", "println 5 <= 4;", 0);
    eqNum("eq1", "println 5 == 5;", 1);
    eqNum("eq0", "println 5 == 6;", 0);
    eqNum("ne1", "println 5 != 6;", 1);
    eqNum("ne0", "println 5 != 5;", 0);
}


static void testCompareString()
{
    // String operands exercise the typeStr branch of each comparison.
    eqNum("str_gt", "println \"b\" > \"a\";",  1);
    eqNum("str_lt", "println \"a\" < \"b\";",  1);
    eqNum("str_ge", "println \"a\" >= \"a\";", 1);
    eqNum("str_le", "println \"a\" <= \"a\";", 1);
    eqNum("str_eq", "println \"a\" == \"a\";", 1);
    eqNum("str_ne", "println \"a\" != \"b\";", 1);
    // Mismatched types: neither the both-double nor both-string branch fires.
    // Exercise both operand orders so the short-circuited type checks are
    // each taken in both directions.
    eqNum("mismatch_gt",  "println \"a\" > 1;", 0);   // op0 string, op1 number
    eqNum("mismatch_gt2", "println 1 > \"a\";", 0);   // op0 number, op1 string
    eqNum("mismatch_lt2", "println 1 < \"a\";", 0);
    eqNum("mismatch_ge2", "println 1 >= \"a\";", 0);
    eqNum("mismatch_le2", "println 1 <= \"a\";", 0);
    eqNum("mismatch_eq2", "println 1 == \"a\";", 0);
    eqNum("mismatch_ne2", "println 1 != \"a\";", 0);
    eqNum("mismatch_add", "println \"a\" + 1;", 0);
}


// Strings longer than MAX_STR_LEN (128) exercise the truncation ternaries
// scattered through the string-producing operations.
static void testLongStrings()
{
    const std::string z160(160, 'Z');
    const std::string a100(100, 'A');
    const std::string b100(100, 'B');

    eqNum("literal_trunc", "x = \"" + z160 + "\"; println strlen(x);", 128);
    eqNum("concat_trunc",
          "a = \"" + a100 + "\"; b = \"" + b100 + "\"; println strlen(a + b);", 128);
    eqNum("substr_trunc",
          "println strlen(substr(\"" + z160 + "\", 0, 160));", 128);
    eqNum("substitute_trunc",
          "println strlen(substitute(\"" + z160 + "\", \"Q\", \"q\"));", 128);
}


static void testLogicBitwise()
{
    eqNum("and1", "println (1 && 1);", 1);
    eqNum("and0", "println (1 && 0);", 0);
    eqNum("or1",  "println (0 || 1);", 1);
    eqNum("or0",  "println (0 || 0);", 0);
    eqNum("not1", "println !0;", 1);
    eqNum("not0", "println !5;", 0);
    eqNum("band", "println 6 & 3;", 2);
    eqNum("bor",  "println 6 | 1;", 7);
    eqNum("shl",  "println 1 << 4;", 16);
    eqNum("shr",  "println 256 >> 2;", 64);
}


static void testMathBuiltins()
{
    eqNum("abs_neg", "println abs(-5);", 5);   // both branches of the abs ternary
    eqNum("abs_pos", "println abs(5);", 5);
    eqNum("acos", "println acos(1);", 0);
    eqNum("asin", "println asin(0);", 0);
    eqNum("atan", "println atan(0);", 0);
    eqNum("atan2", "println atan2(0, 1);", 0);
    eqNum("ceil", "println ceil(3.2);", 4);
    eqNum("cos",  "println cos(0);", 1);
    eqNum("cosh", "println cosh(0);", 1);
    eqNum("exp",  "println exp(0);", 1);
    eqNum("fabs", "println fabs(-3);", 3);
    eqNum("floor","println floor(3.7);", 3);
    eqNum("fmod", "println fmod(10, 3);", 1);
    eqNum("frexp","println frexp(8, e);", 0.5);
    eqNum("ldexp","println ldexp(1, 3);", 8);
    eqNum("log",  "println log(1);", 0);
    eqNum("log10","println log10(1000);", 3);
    eqNum("modf", "println modf(3.5, e);", 0.5);
    eqNum("pow",  "println pow(2, 3);", 8);
    eqNum("sin",  "println sin(0);", 0);
    eqNum("sinh", "println sinh(0);", 0);
    eqNum("sqrt", "println sqrt(9);", 3);
    eqNum("cbrt", "println cbrt(8);", 2);
    eqNum("tan",  "println tan(0);", 0);
    eqNum("tanh", "println tanh(0);", 0);
    eqNum("min_a","println min(3, 8);", 3);   // op0 < op1 branch
    eqNum("min_b","println min(8, 3);", 3);   // op0 > op1 branch
    eqNum("max_a","println max(3, 8);", 8);
    eqNum("max_b","println max(8, 3);", 8);
    eqNum("pi",   "println pi();", 3.14159265, 1e-6);
    runOnly("rand", "println rand();");
}


static void testStrings()
{
    eqStr("concat",     "print \"foo\" + \"bar\";", "foobar");
    eqNum("strlen",     "println strlen(\"hello\");", 5);
    eqStr("substr",     "print substr(\"hello\", 1, 3);", "ell");
    eqStr("replace",    "print replace(\"hello\", \"XY\", 1, 2);", "hXYlo");
    eqStr("substitute", "print substitute(\"aaa\", \"a\", \"b\");", "bbb");
    eqStr("tostr",      "print tostr(42);", "42");
    eqNum("tonum",      "println tonum(\"3.5\") + 1;", 4.5);
    eqNum("match",      "println match(\"abc\", \"a\");", 1);   // stub returns 1
    runOnly("date",     "print date(\"y\");");
}


static void testTernaryIfn()
{
    eqNum("tern_true",  "println 1 ? 10 : 20;", 10);
    eqNum("tern_false", "println 0 ? 10 : 20;", 20);
    eqNum("ifn_num_t",  "println ifn(1, 10, 20);", 10);
    eqNum("ifn_num_f",  "println ifn(0, 10, 20);", 20);
    eqStr("ifn_str_t",  "print ifn(1, \"yes\", \"no\");", "yes");   // string branch, true
    eqStr("ifn_str_f",  "print ifn(0, \"yes\", \"no\");", "no");    // string branch, false
    eqStr("ifs",        "print ifs(1, \"a\", \"b\");", "a");
}


static void testPrecision()
{
    eqNum("setprec_get", "setprec(3); println getprec();", 3);
    eqStr("setprec_fmt", "setprec(2); print pi();", "3.14");
    // Non-positive precision is clamped back to the default (7 decimals).
    eqStr("setprec_neg", "setprec(-1); print 1;", "1.0000000");
}


static void testControl()
{
    eqNum("while",   "s = 0; i = 1; while (i <= 5) { s += i; ++i; } println s;", 15);
    eqNum("for",     "f = 1; for (k = 1; k <= 5; k += 1) { f *= k; } println f;", 120);
    eqNum("if_then", "x = 10; if (x > 5) println 1; else println 0;", 1);
    eqNum("if_else", "x = 1; if (x > 5) println 1; else println 0;", 0);
    eqNum("case_hit",  "y = 2; case (y) { when 1: println 10; when 2: println 20; else: println 0; }", 20);
    eqNum("case_dflt", "y = 9; case (y) { when 1: println 10; else: println 0; }", 0);
    eqNum("case_str",  "s = \"b\"; case (s) { when \"a\": println 1; when \"b\": println 2; else: println 0; }", 2);
}


static void testFunctions()
{
    eqNum("func_call",    "func add(a, b) { return a + b; } println add(3, 4);", 7);
    eqNum("func_recurse", "func fact(n) { if (n <= 1) { return 1; } return n * fact(n - 1); } println fact(5);", 120);
    eqNum("func_local",   "func sq(x) { return x * x; } x = 99; println sq(5);", 25);
    eqNum("func_isolate", "func sq(x) { return x * x; } x = 99; sq(5); println x;", 99);
    eqNum("func_noret",   "func f() { println 7; } f();", 7);
    eqNum("func_named_local", "func f() { total = 11; return total; } println f();", 11);
    eqNum("func_missing_arg", "func h(a, b) { return a; } println h(5);", 5);   // missing-arg branch
    hasStr("undef_func", "println nosuch(1);", "undefined function");           // not-found branch
    eqNum("named_tilde", "total ~ 5; println total;", 5);                        // '~' assignment
    eqNum("undef_named", "println neverset;", 0);                                // auto-init named var
}


static void testLexer()
{
    eqNum("sci_pos",   "println 1.5e3;", 1500);
    eqNum("sci_neg",   "println 2e-2;", 0.02);
    eqNum("dot_start", "println .5;", 0.5);
    eqNum("comment",   "a = 3; // a comment\n println a;", 3);
    eqStr("unterminated", "x = \"abc", "");    // missing closing quote: no crash, no output
    eqStr("empty",     "", "");
    eqStr("semis",     ";;;", "");
    eqNum("named_up",  "A = 7; println A;", 7);   // uppercase name is a named variable
}


// The C++ API surface: the Calc() overloads and the Init/Load/GetRes/Close
// sequence, plus object reuse.
static void testApi()
{
    // Column Calc(Column&).
    {
        Boascript bs;
        Column in;
        in.push_back("print \"one\";");
        in.push_back("println 2 + 2;");
        Column out = bs.Calc(in);
        if (out.size() == 2 && out[0] == "one") ++g_pass;
        else { ++g_fail; std::cout << "FAIL [Calc(Column)]\n"; }
    }

    // void Calc(const Column& in, Column& out) with out larger than in.
    {
        Boascript bs;
        Column in;
        in.push_back("print \"x\";");
        Column out;
        out.push_back("");
        out.push_back("");     // no corresponding input -> "(empty)"
        bs.Calc(in, out);
        if (out[0] == "x" && out[1] == "(empty)") ++g_pass;
        else { ++g_fail; std::cout << "FAIL [Calc(in,out)]: [" << out[0]
                                   << "],[" << out[1] << "]\n"; }
    }

    // Manual Init/Load/yyparse/GetRes/Close.
    {
        Boascript bs;
        bs.Init();
        bs.Load("println 6 * 7;");
        bs.yyparse();
        std::string r = bs.GetRes();
        bs.Close();
        double d = 0.0;
        util::Tokenizer::string2double(r, d);
        if (std::fabs(d - 42.0) < 1e-9) ++g_pass;
        else { ++g_fail; std::cout << "FAIL [Init/Load/Close]: [" << r << "]\n"; }
    }

    // Object reuse must not accumulate output across Calc() calls.
    {
        Boascript bs;
        bs.Calc("print \"A\";");
        std::string second = bs.Calc("print \"B\";");
        if (second == "B") ++g_pass;
        else { ++g_fail; std::cout << "FAIL [reuse]: [" << second << "]\n"; }
    }
}


int main()
{
    testArithmetic();
    testAssignOps();
    testCompareNumeric();
    testCompareString();
    testLongStrings();
    testLogicBitwise();
    testMathBuiltins();
    testStrings();
    testTernaryIfn();
    testPrecision();
    testControl();
    testFunctions();
    testLexer();
    testApi();

    std::cout << "unittests: " << g_pass << " passed, " << g_fail << " failed\n";
    return g_fail == 0 ? 0 : 1;
}
