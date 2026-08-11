/*
 * Generic BoaScript test driver, in the spirit of the ooyacc test suite's
 * shared driver.cpp.
 *
 * It reads a whole BoaScript program -- from the file named as the first
 * argument, or from stdin when no argument is given -- hands it to
 * Boascript::Calc(), and writes the interpreter's output to stdout. That
 * lets a single binary run every golden-file case in cases/ (run the
 * script, diff stdout against <name>.expected).
 */

#include <BTypes.h>
#include <Boascript.tab.h>

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

using namespace BoriSoft;


int main(int argc, char** argv)
{
    std::ostringstream source;

    if (argc > 1)
    {
        std::ifstream in(argv[1]);
        if (!in)
        {
            std::cerr << "driver: cannot open " << argv[1] << std::endl;
            return 2;
        }
        source << in.rdbuf();
    }
    else
    {
        source << std::cin.rdbuf();
    }

    Boascript bs;
    std::cout << bs.Calc(source.str());

    return 0;
}
