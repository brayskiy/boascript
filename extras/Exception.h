/*
 * Exception.h
 *
 *  Created on: Jan 14, 2015
 *      Author: brayskiy
 */

#ifndef _EXCEPTION_H_
#define _EXCEPTION_H_


#include <BTypes.h>
#include <string>


namespace BoriSoft {
namespace exception {


class Exception
{

public:

    Exception() 
        : m_msg((char *)"Generic Error") 
    {}


    Exception(char* msg)
        : m_msg(msg)
    {}


    ~Exception()
    {}


    BINLINE const char* what(void) { return m_msg.c_str(); }

public:

    std::string m_msg;

}; // class Exception


} // namespace exceprion
} // namespace BoriSoft


#endif // _EXCEPTION_H_
