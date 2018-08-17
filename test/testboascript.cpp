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
