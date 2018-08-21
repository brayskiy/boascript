// DateTime.h                    -*-C++-*-


#ifndef INCLUDED_DATETIME
#define INCLUDED_DATETIME


//@PURPOSE: Date and Time.
//
//@CLASSES:
//
//
//@AUTHOR: Boris Rayskiy
//
//@SEE_ALSO:
//
//@DESCRIPTION: . . .
//
///Usage
///-----


#include <string.h>
#include <time.h>
#include <string>


namespace BoriSoft {    
namespace util {


class DateTime
{
    enum DToken
    {
        ENDSTR = 0,
        YEAR,
        MONTH,
        DAY,
        WDAY,
        HOUR24,
        HOUR12,
        MINUTE,
        SECOND,
        MSEC,
        UNKNOWN
    };


    enum DateTimeConst
    {
        SMALLBUFLEN = 8,
        BUFLEN      = 32
    };


public:

    static std::string Get(void)
    {
        tm*    timeinfo;
        time_t rawtime;
        char   buffer[BUFLEN];

        time(&rawtime);
        timeinfo = localtime(&rawtime);

        strftime(buffer, BUFLEN, "%x %X", timeinfo);
        return std::string(buffer);
    }


    static std::string Get(std::string inStr)
    {
        std::string outStr = "";
        int         len    = inStr.size();
        char*       inBuf  = new char[len + 1];
     
        if (!inBuf)
        {
            return outStr;
        }
        memset(inBuf, 0, len + 1);
        memmove(inBuf, inStr.c_str(), len);
        cp = inBuf;
    
        tm*    timeinfo;
        time_t rawtime;
        time(&rawtime);
        timeinfo = localtime(&rawtime);

        int tok;
        while ((tok = parseStr()) != ENDSTR)
        {
            char tmp[SMALLBUFLEN];
            switch (tok)
            {
            case YEAR:
                {
                    if (size == 4)
                    {
                        strftime(tmp, SMALLBUFLEN, "%Y", timeinfo);
                    }
                    else
                    {
                        strftime(tmp, SMALLBUFLEN, "%y", timeinfo);
                    }
                    outStr += tmp;
                }
                break;
            case MONTH:
                {
                    if (size == 3)
                    {
                        strftime(tmp, SMALLBUFLEN, "%b", timeinfo);
                        outStr += tmp;
                    }
                    else
                    {
                        strftime(tmp, SMALLBUFLEN, "%m", timeinfo);
                        outStr += tmp;
                    }
                }
                break;
            case DAY:
                {
                    strftime(tmp, SMALLBUFLEN, "%d", timeinfo);
                    outStr += tmp;
                }
                break;
            case WDAY:
                {
                    strftime(tmp, SMALLBUFLEN, "%a", timeinfo);
                    outStr += tmp;
                }
                break;
            case HOUR24:
                {
                    strftime(tmp, SMALLBUFLEN, "%H", timeinfo);
                    outStr += tmp;
                }
                break;
            case HOUR12:
                {
                    strftime(tmp, SMALLBUFLEN, "%I", timeinfo);
                    outStr += tmp;
                }
                break;
            case MINUTE:
                {
                    strftime(tmp, SMALLBUFLEN, "%M", timeinfo);
                    outStr += tmp;
                }
                break;
            case SECOND:
                {
                    strftime(tmp, SMALLBUFLEN, "%S", timeinfo);
                    outStr += tmp;
                }
                break;
            default:
                {
                    outStr += (char)tok;
                }
            }
        }

        if (inBuf)
        {
            delete [] inBuf;
            inBuf = 0;
        }

        return outStr;
    }

private:

    static int getToken(char tok)
    {
        int i = 0;
        while (cp[i++] == tok);
        return i - 1;
    }


    static int parseStr()
    {
        while ((*cp == '\n') || (*cp == '\t'))
        {
            cp++;
        }

        if (*cp == '\0')
        {
            return ENDSTR;
        }

        char sym = *cp;
        size = getToken(sym);
        cp = cp + size;
        switch (sym)
        {
        case 'y': return YEAR;
        case 'm': return MONTH;
        case 'D': return WDAY;
        case 'd': return DAY;
        case 'H': return HOUR24;
        case 'h': return HOUR12;
        case 'M': return MINUTE;
        case 'S': return SECOND;
        default:  return *(cp - 1);
        }
    }

private:

    static int   size;
    static char* cp;

};


}  // namespace util
}  // namespace BoriSoft


#endif  // INCLUDED_DATETIME

