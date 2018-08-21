#include <BTypes.h>
#include <Boascript.tab.h>
#include <string> 

using namespace BoriSoft;

int main()
{
    std::string src = "print(\"Hello\");";

    Boascript boascript;
    boascript.Calc(src);
    
    return 0;
}
