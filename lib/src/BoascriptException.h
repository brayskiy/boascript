/**********************************************************************

Filename    :   BoaScriptException.h
Content     :   

Created     :   June 20, 2013
Authors     :   Boris Rayskiy

Copyright   :   (c) 2013

**********************************************************************/


#ifndef _BOA_SCRIPT_EXCEPTION_H_
#define _BOA_SCRIPT_EXCEPTION_H_


#include <BException.h>


using namespace BoriSoft;


namespace BoriSoft {
namespace util {

class BoaScriptException : public util::BException
{

public:

	BoaScriptException(std::string msg) : BException(msg) {}

};


} // namespace util
} // namespace BoriSoft


#define THROW_BOA_SCRIPT_EXCEPTION(msg)        \
{                                              \
    std::ostringstream oss;                    \
    oss << "BoaScriptException: ";             \
    oss << __FILE__ << ": ";                   \
    oss << __LINE__ << " ";                    \
    oss << msg;                                \
                                               \
    throw util::BoaScriptException(oss.str()); \
}


#endif // _B_PLOT_EXCEPTION_H_
