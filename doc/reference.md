     1. Basic syntax
        expr     - regular expression what can be presented as string
                    or as num. Each expr has a type - "num" or "string".
        ( expr ) - returns result in braces.

     2. BoaScript is a dynamically typed language. There are no type definitions.
        There are two types in BoaScript: numerical (<b>num</b>) and <b>string</b>.
        
     3. Variables.
        Variables are places that store values. BoaScript has 26 one-character
        names reserved for variables.

     4. Operator of assignment.
        a = num;
        s = string;
        Example:
            a = 2;
            b = sin(a) + pow(2, 4);
            print b + 1;

     5. Arithmetic operations.
        num - evaluates regular expression as numerical.
        num + num - returns sum;
        num - num - returns difference;
        num * num - returns production of two expressions;
        num / num - returns result of devision;
        num % num - returns rest of division of two integers;
        Example:
            print 1 + 2 * (4.5 - 8.9) / 3.14 + 5 % 2;

     6. Operations with strings
        string + string - joins two strings;
        Example:
            s = "Hello";
            t = " world";
            print s + t;

     7. Logical operations and bit operations.
        The result of logical operation is "1" if thrue or "0" if false.
        Note: for correct and predictable evaluation use integer constants.
        num == num
        num <  num;
        num <= num;
        num >  num;
        num >= num;
        num != num;
        num || num;
        num && num;
        Example:
           a = 2;
           b = 3;
           print a > b;
           print a < b;
           c = (a == b) + 1;

        num |  num  - bit OR;
        num &  num  - bit AND;
        num !  num  - bit NOT;
        num << num  - left bit shift;
        num >> num  - right bit shift;
        Example:
            print 1 << 2;
            print !1;

        string == string;
        string <  string;
        string <= string;
        string >= string;
        string != string;
        Example:
            print "A" == "A";
            a = "A" > "B";

     8. Functions.
        - returning num 
        abs(num)                 -
        acos(num)                -
        asin(num)                -
        atan(num)                -
        atan2(num, num)          -
        ceil(num)                -
        cos(num)                 -
        cosh(num)                -
        cosh(num)                -
        exp(num)                 -
        fabs(num)                - 
        floor(num)               -
        fmod(num, num)           -
        frexp(num, num)          -
        log(num)                 - natural logarithm;
        log10(num)               - logarithm of base 10;
        modf(num, num)           -
        pow(num, num)            - power;
        num ^ num                - also power;
        sin(num)                 -
        sinh(num)                -
        sqrt(num)                -
        cbrt(num)                - fast cubic root approximation
                                   (experimental);
        tan(num)                 - tangens;
        tanh(num)                -
        min(num, num)            - minimal value among two expressions;
        max(num, num)            - maximal value among two expressions;
        rand()                   - random number generator in the range 0~1;
    
        Example:
            print sqrt(2);
            a = sin(sqrt(cos(0) + 1)) + atan(sin(3.1415926 / 2));
            b = rand();
        
        strlen(string)           - returns length of string;
        Example:
            a = strlen("Hello world");
            print strlen("AAAAAAAA");

        tonum(string)            - converts string to number.
        Example:
            a = tonum("1234567");
            print a;

        - returning strings;
        substr(string, num, num) - returns substring defind by indexes;
        Example:
            s = "Hello";
            print substr(s, 0, strlen(s) - 2);

        replace(string, string, num, num)  
                                 - replases part of string with another one;
        Example:
            s = "Hello";
            t = replace(s, "pp", 2, 2);

        tostring(num)            - converts number to string.
        Example:
            s = tostring(1234567);
            print s;

        date(string) - returns date or/and current time
        string       - format of output date.
        Example:
            s = date("dd-mm(mmm)-yy(yyyy) hh:MM:SS"); print s;
            s = date("mm/dd/yy hh:MM"); print s;
            where
               d - day of month [0...31];
               m - month [1...12] (mm format) or abbrev. of month (mmm format);
               y - year (two digits or four digits);
               h - hour of day [1...12];
               H - hour of day [0...23];
               M - minute [0...59];
               S - second [0...59];

        substitute(string, string, string) - substitutes target string instead
                                             the pattern in the given string, 
                                             i.e substitute(instr, pattern,
                                                            target);
        Example:
            s = substr(a, 0, 3);
            t = $1;
            u = s + " " + t;
            v = ifs(!match(u, "BBG"), 
                    substitute(u, "BBG", "Something instead"), 
                    "Something else");
            print v;


        match(string, string)              - parses the regular expression,
                                             defined in the second parameter
                                             and returns "1" if the string
                                             (first argument) matchs the
                                             pattern or "0" otherwise.
        Example:
            s = "portfolio_file_1.csv";
            a = match(s, "portfolio([a-zA-Z_]*)_([1-3])(.csv)");
            print a; // 1


     9. Operators 
        expr ? expr : expr
        Example:
            s = "A" > "B" ? "Yes" : "No";
            b = 1 < 2;
            a = b ? 5 : 10;

        ifn(num, num, num)
        Example:
            a = ifn(1 > 2, 6, 7);

        ifs(num, string, string)
        Example:
            s = ifs(1 < 2, "Yes", "No");
            t = ifs("A" >= "B", "True", "False");

        Operator "if"
        Example:
            if (num)
            {
                expr
            }
            else
            {
                expr
            }
        or
            if (num) { expr }

        Operator "while"
        Example:
            while (expr)
            {
                expr
            }
        
     10. Comments.
         Line or part of line will be ignored by the interpreter from double
         slash to the end of line.
         Example:
             // This is comment.
             a = 2; // Assignment.
             print a;
             // End of script.
                                                                             -->

    <!-- Illustrates basic functionality of the calculator                   -->
