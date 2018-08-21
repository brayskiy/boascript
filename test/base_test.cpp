#include <BTypes.h>
#include <Boascript.tab.h>
#include <string> 
#include <iostream>

#define TEST_DEBUG

using namespace BoriSoft;

int main()
{
    std::string src = "print(\"Hello\");";

    Boascript boascript;
    std::string& res =  boascript.Calc(src);
    
    std::cout << res << std::endl;
    
    return 0;
}
