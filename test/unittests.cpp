/*
 * Focused unit tests for BoaScript, exercising the interpreter API and
 * every language construct, builtin, operator, and error path. Together
 * with testboascript.cpp and the golden-file cases these drive the
 * coverage measured by coverage.sh.
 *
 * Self-contained: no framework. Each check compares Boascript::run output
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
    return bs.run(src);
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

// Assert a predicate on the program's output string.
static void check(const char* what, const std::string& src, bool ok)
{
    if (ok) { ++g_pass; return; }
    ++g_fail;
    std::cout << "FAIL [" << what << "]: unexpected output [" << run(src) << "]\n";
}

// The version string must look like YY.WW.BB: three dot-separated numbers.
static bool looksLikeVersion(const std::string& s)
{
    int parts = 1;
    for (size_t i = 0; i < s.size(); ++i)
    {
        if (s[i] == '.') { ++parts; continue; }
        if (s[i] < '0' || s[i] > '9') return false;
    }
    return parts == 3 && s.size() >= 5;   // e.g. "0.0.0"
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
    // '+' with mixed string/number operands concatenates (number stringified).
    eqStr("concat_str_num", "print \"a\" + 1;", "a1");
    eqStr("concat_num_str", "print 1 + \"a\";", "1a");
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


static void testIntgauss3()
{
    // 3-point Gauss-Legendre is exact for polynomials up to degree 5.
    eqNum("ig_x2",   "func f(x) { return x * x; } println intgauss3(f, 0, 1);", 1.0 / 3.0);
    eqNum("ig_x3",   "func g(x) { return x * x * x; } println intgauss3(g, 0, 2);", 4.0);
    eqNum("ig_poly", "func p(x) { return 3*x*x + 2*x + 1; } println intgauss3(p, 0, 1);", 3.0);
    eqNum("ig_x5",   "func q(x) { return x*x*x*x*x; } println intgauss3(q, 0, 1);", 1.0 / 6.0);
    eqNum("ig_sin",  "func s(x) { return sin(x); } println intgauss3(s, 0, pi());", 2.001389, 1e-5);
    // A single-letter function name (VARIABLE form of the rule).
    eqNum("ig_single", "func a(x) { return x * x; } println intgauss3(a, 0, 1);", 1.0 / 3.0);
    // An undefined integrand is reported once, not crashed on.
    hasStr("ig_undef", "println intgauss3(nofunc, 0, 1);", "undefined function");
}


static void testNumberBases()
{
    eqNum("hex_FF",  "println 0xFF;", 255);
    eqNum("hex_low", "println 0xff;", 255);
    eqNum("hex_10",  "println 0x10;", 16);
    eqNum("hex_0",   "println 0x0;", 0);
    eqNum("bin",     "println 0b1010;", 10);
    eqNum("bin_8",   "println 0b1000;", 8);
    eqNum("oct",     "println 0o17;", 15);
    eqNum("oct_777", "println 0o777;", 511);
    eqNum("hex_arith", "println 0xFF + 1;", 256);
    // A leading zero that is not a base prefix falls through to decimal.
    eqNum("leading_zero", "println 08;", 8);
    eqNum("plain_zero",   "println 0 + 5;", 5);
}


static void testHypotUpperLower()
{
    eqNum("hypot_345",  "println hypot(3, 4);", 5);
    eqNum("hypot_512",  "println hypot(5, 12);", 13);
    eqNum("hypot_zero", "println hypot(0, 0);", 0);
    eqStr("upper",       "print upper(\"hello World\");", "HELLO WORLD");
    eqStr("lower",       "print lower(\"Hello WORLD\");", "hello world");
    eqStr("upper_digits","print upper(\"abc123\");", "ABC123");
    eqStr("lower_empty", "print lower(\"\");", "");
}


static void testStringManip()
{
    eqNum("len_str",   "println len(\"hello\");", 5);
    eqNum("len_empty", "println len(\"\");", 0);
    eqStr("reverse",   "print reverse(\"abcdef\");", "fedcba");
    eqStr("reverse_pal","print reverse(\"racecar\");", "racecar");
    eqNum("find_hit",  "println find(\"Hello, World\", \"World\");", 7);
    eqNum("find_miss", "println find(\"Hello\", \"xyz\");", -1);
    eqNum("find_start","println find(\"abc\", \"a\");", 0);
    eqStr("repeat",    "print repeat(\"ab\", 3);", "ababab");
    eqStr("repeat0",   "print repeat(\"ab\", 0);", "");
    eqStr("charat",    "print charat(\"Hello\", 1);", "e");
    eqStr("charat_oob","print charat(\"Hi\", 9);", "");
    // Composition and mixed concatenation.
    eqStr("upper_substr", "print upper(substr(\"Hello, World\", 7, 5));", "WORLD");
    eqNum("len_reverse",  "println len(reverse(\"hello\"));", 5);
    eqStr("concat_label", "print \"count=\" + 42;", "count=42");
    eqStr("concat_two",   "print \"foo\" + \"bar\";", "foobar");
}


static void testArrays()
{
    eqNum("arr_len",  "a = [1, 2, 3, 4, 5]; println len(a);", 5);
    eqNum("arr_sum",  "a = [1, 2, 3, 4, 5]; println sum(a);", 15);
    eqNum("arr_avg",  "a = [1, 2, 3, 4, 5]; println avg(a);", 3);
    eqNum("arr_prod", "a = [1, 2, 3, 4, 5]; println prod(a);", 120);
    eqNum("arr_max",  "a = [1, 9, 3]; println max(a);", 9);
    eqNum("arr_min",  "a = [4, 1, 3]; println min(a);", 1);
    eqNum("arr_get0", "a = [10, 20, 30]; println a[0];", 10);
    eqNum("arr_get2", "a = [10, 20, 30]; println a[2];", 30);
    eqNum("arr_oob",  "a = [1, 2]; println a[9];", 0);          // out of range -> 0
    eqNum("arr_set",  "a = [1, 2, 3]; a[1] = 99; println a[1];", 99);
    eqNum("arr_expr_elems", "e = [1 + 1, 2 * 3, 4 - 1]; println sum(e);", 11);
    eqNum("arr_empty_len",  "c = []; println len(c);", 0);
    eqNum("arr_empty_sum",  "c = []; println sum(c);", 0);
    eqNum("arr_empty_max",  "c = []; println max(c);", 0);
    // Whole-array printing uses the nested bracket form.
    eqStr("arr_print",  "a = [1, 2, 3]; print a;", "[1, 2, 3]");
    // Multi-dimensional arrays.
    eqStr("arr_2d_print", "m = [[1, 2], [3, 4]]; print m;", "[[1, 2], [3, 4]]");
    eqNum("arr_2d_get",   "m = [[1, 2, 3], [4, 5, 6]]; println m[1][2];", 6);
    eqNum("arr_2d_len",   "m = [[1, 2, 3], [4, 5, 6]]; println len(m);", 2);
    eqNum("arr_2d_leninner","m = [[1, 2, 3], [4, 5, 6]]; println len(m[0]);", 3);
    eqNum("arr_2d_sum",   "m = [[1, 2, 3], [4, 5, 6]]; println sum(m);", 21);
    eqNum("arr_2d_set",   "m = [[1, 2], [3, 4]]; m[1][0] = 30; println m[1][0];", 30);
    eqNum("arr_3d_get",   "c = [[[1, 2], [3, 4]], [[5, 6], [7, 8]]]; println c[1][0][1];", 6);
    eqNum("arr_3d_sum",   "c = [[[1, 2], [3, 4]], [[5, 6], [7, 8]]]; println sum(c);", 36);
    eqNum("arr_expr_nest","n = [[1 + 1, 2 * 2], [3 * 3, 4 + 4]]; println sum(n);", 23);
    // Nested reductions fold over all leaves; len is the outer dimension.
    eqNum("arr_2d_prod",  "m = [[1, 2], [3, 4]]; println prod(m);", 24);
    eqNum("arr_2d_avg",   "m = [[1, 2], [3, 4]]; println avg(m);", 2.5);
    eqNum("arr_2d_max",   "m = [[1, 9], [3, 4]]; println max(m);", 9);
    eqNum("arr_2d_min",   "m = [[5, 2], [3, 4]]; println min(m);", 2);
    // Extracting a sub-array into a variable.
    eqNum("arr_subarray", "m = [[1, 2, 3], [4, 5, 6]]; r = m[1]; println sum(r);", 15);
    eqStr("arr_sub_print","m = [[1, 2, 3], [4, 5, 6]]; r = m[0]; print r;", "[1, 2, 3]");
    // Copying an array by name.
    eqNum("arr_copy",     "a = [1, 2, 3]; b = a; b[0] = 9; println sum(a) + sum(b);", 6 + 14);
    // Reassigning an array name to a scalar sheds the array binding.
    eqNum("arr_to_scalar","a = [1, 2, 3]; a = 5; println a + 1;", 6);
    // Out-of-range element assignment is ignored.
    eqNum("arr_set_oob",  "a = [1, 2, 3]; a[9] = 100; println sum(a);", 6);
    // Multi-character array names.
    eqNum("arr_named", "data = [2, 4, 6]; println sum(data);", 12);
    // The 2-argument numeric max/min still work.
    eqNum("num_max2", "println max(3, 8);", 8);
    eqNum("num_min2", "println min(8, 3);", 3);
    // Reduction words remain usable as ordinary variables.
    eqNum("sum_as_var", "sum = 7; println sum + 1;", 8);
    eqNum("len_as_var", "len = 5; println len * 2;", 10);
}


static void testBoolean()
{
    // Comparison and logical (already present) plus true/false literals.
    eqNum("true_lit",  "println true;", 1);
    eqNum("false_lit", "println false;", 0);
    eqNum("and_tf",    "println (true && false);", 0);
    eqNum("or_tf",     "println (true || false);", 1);
    eqNum("not_true",  "println !true;", 0);
    eqNum("cmp_lt",    "println 3 < 5;", 1);
    eqNum("cmp_eq",    "println 4 == 4;", 1);
    eqNum("cmp_ne",    "println 4 != 4;", 0);
    // Bitwise xor() and bitnot().
    eqNum("xor_a",     "println xor(12, 10);", 6);
    eqNum("xor_b",     "println xor(255, 15);", 240);
    eqNum("xor_zero",  "println xor(5, 5);", 0);
    eqNum("bitnot_0",  "println bitnot(0);", -1);
    eqNum("bitnot_5",  "println bitnot(5);", -6);
    // true/false remain usable in arithmetic (they are 1 and 0).
    eqNum("true_plus", "println true + true + false;", 2);
}


static void testMatrix()
{
    eqNum("det_2x2",   "a = [[1, 2], [3, 4]]; println det(a);", -2);
    eqNum("det_3x3",   "b = [[1,2,3],[4,5,6],[7,8,10]]; println det(b);", -3);
    eqNum("det_diag",  "c = [[2, 0], [0, 4]]; println det(c);", 8);
    // inverse: check a couple of elements.
    eqNum("inv_00",    "a = [[1, 2], [3, 4]]; m = inverse(a); println m[0][0];", -2);
    eqNum("inv_11",    "a = [[1, 2], [3, 4]]; m = inverse(a); println m[1][1];", -0.5);
    eqStr("inv_print", "a = [[1, 2], [3, 4]]; print inverse(a);",
          "[[-2, 1], [1.5, -0.5]]");
    // rotate 90 degrees clockwise.
    eqStr("rotate",    "a = [[1, 2], [3, 4]]; print rotate(a);", "[[3, 1], [4, 2]]");
    eqStr("rotate2",   "a = [[1, 2], [3, 4]]; print rotate(rotate(a));", "[[4, 3], [2, 1]]");
    // submatrix (minor), and composition with det.
    eqStr("submatrix", "b = [[1,2,3],[4,5,6],[7,8,10]]; print submatrix(b, 0, 0);",
          "[[5, 6], [8, 10]]");
    eqNum("det_of_sub","b = [[1,2,3],[4,5,6],[7,8,10]]; println det(submatrix(b, 0, 0));", 2);
    // solve a linear system.
    eqStr("solve2",    "print solve([[2, 1], [1, 3]], [3, 5]);", "[0.8, 1.4]");
    eqStr("solve3",    "print solve([[2,1,-1],[-3,-1,2],[-2,1,2]], [8,-11,-3]);",
          "[2, 3, -1]");
    // matrix-builtin result is a normal array: assign, index, reduce.
    eqNum("mat_sum",   "a = [[1, 2], [3, 4]]; m = rotate(a); println sum(m);", 10);
}


static void testVersion()
{
    // version() returns a YY.WW.BB string; about()/description() the app text.
    check("version_fmt", "print version();", looksLikeVersion(run("print version();")));
    check("about_nonempty", "print about();", run("print about();").size() > 0);
    check("about_boascript", "print about();",
          run("print about();").find("BoaScript") != std::string::npos);
    check("description_alias", "print description();",
          run("print description();") == run("print about();"));
    // Usable in expressions; version/about are not reserved words.
    check("version_concat", "print \"v\" + version();",
          run("print \"v\" + version();").substr(0, 1) == "v");
    eqNum("version_as_var", "version = 5; println version + 1;", 6);
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


// The C++ API surface: the run() overloads and the Init/Load/GetRes/Close
// sequence, plus object reuse.
static void testApi()
{
    // Column run(Column&).
    {
        Boascript bs;
        Column in;
        in.push_back("print \"one\";");
        in.push_back("println 2 + 2;");
        Column out = bs.run(in);
        if (out.size() == 2 && out[0] == "one") ++g_pass;
        else { ++g_fail; std::cout << "FAIL [run(Column)]\n"; }
    }

    // void run(const Column& in, Column& out) with out larger than in.
    {
        Boascript bs;
        Column in;
        in.push_back("print \"x\";");
        Column out;
        out.push_back("");
        out.push_back("");     // no corresponding input -> "(empty)"
        bs.run(in, out);
        if (out[0] == "x" && out[1] == "(empty)") ++g_pass;
        else { ++g_fail; std::cout << "FAIL [run(in,out)]: [" << out[0]
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

    // Object reuse must not accumulate output across run() calls.
    {
        Boascript bs;
        bs.run("print \"A\";");
        std::string second = bs.run("print \"B\";");
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
    testIntgauss3();
    testNumberBases();
    testHypotUpperLower();
    testStringManip();
    testArrays();
    testBoolean();
    testMatrix();
    testVersion();
    testLexer();
    testApi();

    std::cout << "unittests: " << g_pass << " passed, " << g_fail << " failed\n";
    return g_fail == 0 ? 0 : 1;
}
