#include <BTypes.h>
#include <Boascript.tab.h>
#include <string>

#define TEST_DEBUG

using namespace BoriSoft;


#ifdef CALC_BATCH

typedef bool (*assertFunc)(const char*, const char*);

struct TestCase
{
    UInt        id;
    const char* test;
    const char* check;
    assertFunc  func;
};


bool assertEqualString(const char* test, const char* check)
{
    Boascript bs;
    std::string& res = bs.Calc(test);
#ifdef TEST_DEBUG
    std::cout << res << " = " << check << std::endl;
#endif
    return res == std::string(check);
}

bool assertLikeString(const char* test, const char* check)
{
    Boascript bs;
    std::string& res = bs.Calc(test);
#ifdef TEST_DEBUG
    std::cout << res << std::endl;
#endif
    return res.find(check) != std::string::npos;
}

bool assertEqualDouble(const char* test, const char* check)
{
    Boascript bs;
    //bs.Init();
    //bs.Load(test);
    //bs.yyparse(); 

    Double res;
    //util::Tokenizer::string2double(bs.GetRes(), res);
    util::Tokenizer::string2double(bs.Calc(test), res);
    Double chk;
    util::Tokenizer::string2double(check, chk);
#ifdef TEST_DEBUG
    std::cout << res << " = " << chk << std::endl;
#endif
    //bs.Close();

    return res == chk;
}



TestCase testCase[] =
{
    {
        1,
        "println 1;",
        "1",
        assertEqualDouble 
    },

    {
        2,
        " a = 1.2345e-01 println a;",
        "0.1234500",
        assertEqualDouble
    },

    {
        3,
        "i = 1;                                                        "
        "while (i < 10) { if (i % 2 != 0) { ++i; } else { ++i; ++i; } }"
        "println i;                                                    ",
        "10",
        assertEqualDouble
    },

    {
        4,
        "a = 2.3456e-50; println log(a);",
        "-114.2767134",
        assertEqualDouble
    },

    {
        5,
        "d = 2;                                                "
        "e = sqrt(sqrt(sqrt(sqrt(sqrt(sqrt(sqrt(sqrt(d))))))));"
        "f = d ^ (1/(2 ^ 8));                                  "
        "println e - f;                                        ",
        "0",
        assertEqualDouble
    },

    {
        6,
        "s = replace(\"Hello world\",                          "
        "            \"pp\", 2, 2);                            "
        "print s;                                              ",
        "Heppo world",
        assertEqualString
    },

    {
        7,
        "s = \"One\";                                          " 
        "t = \"Two\";                                          " 
        "print s + t;                                          ",
        "OneTwo",
        assertEqualString
    },

    {
        8,
        "\n\n // sdf \n x = \" sweet victory\"; // cc \n print \"Hello\" + x;",
        "Hello sweet victory",
        assertEqualString
    },

    {
        9,        
        "x = 0; b = 2; s = (b - x) / 500; while (x <= b) { y = sin(x); x +=s; } println x;",
        "2",
        assertEqualDouble
    },
    
    {
        10,        
        "x = 0; b = 2; s = (b - x) / 500; while (x <= b) { y = s; x +=s; } println x;",
        "2",
        assertEqualDouble
    },
    
    {
        11,
        "x = 0; b = 2; s = (b - x) / 500; while (x <= b) { y = a; x +=s; } println x;",
        "2",
        assertEqualDouble
    },
    
    {
        1200,
        "22e3dfff",
        "yntax error",
        assertLikeString
    },
    
    {
        1202,
        "This is wrong code",
        "yntax error",
        assertLikeString
    },
    
    {
        13,
        "s = substitute(\"Gary BorisGaryGary\", \"Gary\", \"Ruiyang\"); print s;",
        "Ruiyang BorisRuiyangRuiyang",
        assertEqualString
    },
    
    {
        14,
        "a = .0025; println 30 * pow(1 + a, 300 * 12);",
        "240377.5512780",
        assertEqualDouble
    },
    
    {
        15,
        "a = sqrt(3); println a; //aa  \n",
        "1.7320508",
        assertEqualDouble
    },
    
    {
        16,
        "a = 2; b = 3; c = a * b; d = sqrt(c); println d; // Comment is here\n",
        "2.4494897",
        assertEqualDouble
    },
    
    {
        17,
        "setprec(20); a = sqrt(2); print a;",
        "1.41421356237309514547",
        assertEqualDouble
    },
    
    {
        18,
        "setprec(5); a = sqrt(2); println a;",
        "1.41421",
        assertEqualDouble
    },
    
    {
        1000,
        "                                                \n",
        "",
        assertEqualString
    },
    
    {
        1002,
        ";                                               \n",
        "",
        assertEqualString
    },
    
    {
        1004,
        "a;                                               \n",
        "",
        assertEqualString
    },
   
/* 
    {
        1005,
        "a;;;;;;;println a;                               \n",
        "0.0000000",
        assertEqualDouble
    },
*/
  
    {
        1006,
        "a = 0;;;;;;;println a;                          \n",
        "0",
        assertEqualDouble
    },
    
    {
        1008,
        ";;;;;;;println a;;;;;;;;;;                     \n",
        "0",
        assertEqualDouble
    },
    
    {
        2002,
        "b = 20; \n"
        "if (b > 1) if (b > 2) if (b > 3) if (b > 4) if (b > 5) if (b < 21)        \n"
        "if (b > 6) if (b > 7) if (b > 8) if (b > 9) if (b < 22) if (b < 23)       \n"
        "if (b >= 6) if (b >= 7) if (b >= 8) if (b >= 9) if (b <= 22) if (b <= 23) \n"
        "print b;",
        "20",
        assertEqualDouble
    },
    
    {
        2004,
        "b = 10; if (b != 15) if (b != 14) if (b != 13) if (b != 12) if (b != 11) print b;",
        "10",
        assertEqualDouble
    },
    
    {
        2006,
        "b = 15; if ((b != 15) && (b != 14) && (b != 13) && (b != 12))"
        "print b; else print b * 2;",
        "30",
        assertEqualDouble
    },
    
    {
        2208,
        "setprec(31); print pi();",
        "3.1415926535897932384626433832795",
        assertEqualDouble
    },

    // Multi-character (named) variables.

    {
        3000,
        "total = 42; println total;",
        "42",
        assertEqualDouble
    },

    {
        3002,
        "price = 10; qty = 3; println price * qty;",
        "30",
        assertEqualDouble
    },

    {
        3004,
        "sum = 0; i = 1; while (i <= 5) { sum += i; ++i; } println sum;",
        "15",
        assertEqualDouble
    },

    {
        3006,
        "count = 5; count *= 3; count %= 4; println count;",
        "3",
        assertEqualDouble
    },

    {
        3008,
        "name = \"Boris\"; print name + \" R\";",
        "Boris R",
        assertEqualString
    },

    {
        3010,
        "a = 2; result = a * 10; println result;",
        "20",
        assertEqualDouble
    },

    // for loops.

    {
        4000,
        "f = 1; for (k = 1; k <= 5; k += 1) { f *= k; } println f;",
        "120",
        assertEqualDouble
    },

    {
        4002,
        "t = 0; for (i = 1; i <= 3; i += 1) "
        "{ for (j = 1; j <= 3; j += 1) { t += i * j; } } println t;",
        "36",
        assertEqualDouble
    },

    // User-defined functions.

    {
        4004,
        "func add(a, b) { return a + b; } println add(3, 4);",
        "7",
        assertEqualDouble
    },

    {
        4006,
        "func fact(n) { if (n <= 1) { return 1; } return n * fact(n - 1); } "
        "println fact(5);",
        "120",
        assertEqualDouble
    },

    {
        4008,
        "func sq(x) { return x * x; } x = 99; println sq(5) + x;",
        "124",
        assertEqualDouble
    },

    // case / when.

    {
        4010,
        "y = 3; case (y) { when 1: println 100; when 2: println 200; "
        "when 3: println 300; else: println 0; }",
        "300",
        assertEqualDouble
    },

    // Gauss-Legendre integration of x^2 over [0,1] = 1/3, written in
    // BoaScript as a user function calling another user function.
    {
        4012,
        "func f(x) { return x * x; } "
        "func gaussint(a, b) { "
        "  h = (b - a) / 2; c = (a + b) / 2; s = sqrt(0.6); "
        "  return h * (5.0/9.0*f(c - h*s) + 8.0/9.0*f(c) + 5.0/9.0*f(c + h*s)); "
        "} println gaussint(0, 1);",
        "0.3333333",
        assertEqualDouble
    },

    // The intgauss3 builtin: integral of x^2 over [0,1] = 1/3.
    {
        4014,
        "func f(x) { return x * x; } println intgauss3(f, 0, 1);",
        "0.3333333",
        assertEqualDouble
    },

    // Number-base literals and hypot/upper/lower builtins.

    {
        4016,
        "println 0xFF + 0b1010 + 0o17;",   // 255 + 10 + 15
        "280",
        assertEqualDouble
    },

    {
        4018,
        "println hypot(3, 4);",
        "5",
        assertEqualDouble
    },

    {
        4020,
        "print upper(\"boa\") + lower(\"SCRIPT\");",
        "BOAscript",
        assertEqualString
    },

    // Arrays: reductions and element mutation.

    {
        4022,
        "a = [1, 2, 3, 4, 5]; println sum(a);",
        "15",
        assertEqualDouble
    },

    {
        4024,
        "a = [1, 2, 3]; a[1] = 20; println a[0] + a[1] + a[2];",
        "24",
        assertEqualDouble
    },

    {
        4026,
        "a = [3, 1, 4, 1, 5]; println max(a) + min(a) + len(a);",
        "11",
        assertEqualDouble
    },

    // String manipulation.

    {
        4028,
        "print reverse(\"abc\") + repeat(\"-\", 2) + charat(\"XYZ\", 2);",
        "cba--Z",
        assertEqualString
    },

    {
        4030,
        "println find(\"hello world\", \"world\");",
        "6",
        assertEqualDouble
    },

    {
        4032,
        "print \"n=\" + 7;",   // number stringified in concatenation
        "n=7",
        assertEqualString
    },

    // Multi-dimensional arrays.

    {
        4034,
        "m = [[1, 2, 3], [4, 5, 6]]; println sum(m);",
        "21",
        assertEqualDouble
    },

    {
        4036,
        "m = [[1, 2], [3, 4]]; m[1][0] = 30; println m[0][1] + m[1][0];",
        "32",
        assertEqualDouble
    },

    {
        4038,
        "c = [[[1, 2], [3, 4]], [[5, 6], [7, 8]]]; println c[1][0][1] + len(c);",
        "8",
        assertEqualDouble
    },

    {
        4040,
        "m = [[1, 2, 3], [4, 5, 6]]; print m;",
        "[[1, 2, 3], [4, 5, 6]]",
        assertEqualString
    },

};


void runTest(const TestCase& testCase)
{
    if (!testCase.func(testCase.test, testCase.check))
    {
        std::cout << "Test #" << testCase.id << " failed " << std::endl;
        exit (1);
    }
}

#endif // CALC_BATCH


int main(int argc, char** argv)
{

#ifndef CALC_BATCH

    Boascript c;
    c.yyparse();

#else
      
    int numOfCases = sizeof(testCase) / sizeof(testCase[0]);

    std::cout << numOfCases << std::endl;

    for (int i = 0; i < numOfCases; ++i)
    {
        runTest(testCase[i]);
    }

    Boascript c;

    // First part

    static const char* str[] =
    {
        "a = sqrt(2); println a; //aa  \n",
        "a = sqrt(3); println a; //aa  \n",
        "s = \"Hello world\"; print s; \n",
    };

    int n = sizeof(str) / sizeof(str[0]);

    for (int i = 0; i < n; ++i)
    {
        //Boascript bs;
        c.Init();
        c.Load(str[i]);
        c.yyparse();
        std::cout << c.GetRes() << std::endl;
        c.Close();
        
        //std::cout << bs.Calc(str[i]) << std::endl;
    }

    // Third part.

    {
        Boascript y;
        Column in;
        in.push_back("s = \"First  Item\"; println s;");
        in.push_back("t = \"Second Item\"; println t;");
        in.push_back("a = sqrt(2); println a;");
        Column out = y.Calc(in);
        for (ColIt it = out.begin(); it != out.end(); ++it)
        {
            std::cout << *it << std::endl;
        }
    }

    // Forth part.

    {
        Boascript y;

        Column in;
        in.push_back("s = \"Boris\";  print s;");
        in.push_back("t = \"Garry\";  print t;");
        in.push_back("a = \"Bhavin\"; print a;");

        Column out;
        out.push_back("");
        out.push_back("");
        out.push_back("");
        y.Calc(in, out);
        for (ColIt it = out.begin(); it != out.end(); ++it)
        {
            std::cout << *it << std::endl;
        }
    }
    
    {
        Double xMin = 0.0;
        Double xMax = 2.0;
        
        std::string func1 = "a";
        
        Boascript y;
        
        std::ostringstream boaScript;
        boaScript << "x = " << xMin          << ";" << std::endl;
        boaScript << "b = " << xMax          << ";" << std::endl;
        Double step = (xMax - xMin) / 500;  
        boaScript << "s = " << step          << ";" << std::endl;          
        boaScript << "while (x <= b)"               << std::endl;
        boaScript << "{"                            << std::endl;
        boaScript << "    y = " << func1     << ";" << std::endl;
        boaScript << "    print x"           << ";" << std::endl;  
        boaScript << "    print \"@\""       << ";" << std::endl;
        boaScript << "    print y"           << ";" << std::endl;
        boaScript << "    print \"@\""       << ";" << std::endl;
        boaScript << "    x += s"            << ";" << std::endl; 
        boaScript << "}"                            << std::endl;  
        
        std::string& out = y.Calc(boaScript.str());
        //std::cout << out << std::endl;
    }
    
#endif // CALC_BATCH

    return 0;

}
