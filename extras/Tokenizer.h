/**********************************************************************

Filename    :   Tokenizer.h
Content     :   Tokenizing utilities.

Created     :   January 22, 2010
Authors     :   Boris Rayskiy

Copyright   :   (c) 2010

**********************************************************************/


#ifndef _TOKENIZER_H_
#define _TOKENIZER_H_


#include <BTypes.h>
#include <iostream>
#include <string>
#include <vector>
#include <sstream>


namespace BoriSoft {    
namespace util {


class Tokenizer
{

public:

    /*
    template < class T > void tokenize(const std::string& str, T& tokens,
              const std::string& delimiters = " ", bool trimEmpty = false)
    {
        std::string::size_type pos, lastPos = 0, length = str.length();

        using value_type = typename T::value_type;
        using size_type  = typename T::size_type;

        while (lastPos < length + 1)
        {
            pos = str.find_first_of(delimiters, lastPos);
            if (pos == std::string::npos)
            {
                pos = length;
            }

            if (pos != lastPos || !trimEmpty) {
                tokens.push_back(value_type(str.data() + lastPos, (size_type)pos-lastPos));
            }

            lastPos = pos + 1;
        }
    }
    */


    static void tokenize1(const Cell& source,
                          Column&     results,
                          const char* delimiter = " ",
                          bool        keepEmpty = false)
    {
        size_t prev = 0;
        size_t next = 0;

        while ((next = source.find_first_of(delimiter, prev)) != std::string::npos)
        {
            if (keepEmpty || (next - prev != 0))
            {
                results.push_back(source.substr(prev, next - prev));
            }
            prev = next + 1;
        }

        if (prev < source.size())
        {
            results.push_back(source.substr(prev));
        }
    }


    static void tokenize(const std::string& inStr,
                         std::vector<int>& tokens,
                         const std::string& delimiters = " ")
    {
        // Skip delimiters at beginning.
        std::string::size_type lastPos = inStr.find_first_not_of(delimiters, 0);
        // Find first "delimiter".
        std::string::size_type pos     = inStr.find_first_of(delimiters, lastPos);

        while (std::string::npos != pos || std::string::npos != lastPos)
        {
            // Found a token, add it to the vector.

            std::istringstream stream(inStr.substr(lastPos, pos - lastPos));
            int val;
            stream >> val;
            tokens.push_back(val);
            
            // Skip delimiters.  Note the "not_of"
            lastPos = inStr.find_first_not_of(delimiters, pos);
            // Find next "non-delimiter"
            pos = inStr.find_first_of(delimiters, lastPos);
        }
    }

    
    static void tokenize(const std::string& str,
                         Column&            tokens,
                         const Cell&        delimiters = " ",
                         const UInt32       tokensSize = 256)
    {
        std::string::size_type start = 0;
        std::string::size_type end   = 0;

        while (end != std::string::npos)
        {
            end = str.find(delimiters, start); 

            if (end != start)
            {
                std::string out = trim(str.substr(start,
                                                  (end == std::string::npos) ? 
                                                  std::string::npos :
                                                  end - start));
                if (out.size())
                {
                    tokens.push_back(out);
                    if (tokens.size() == tokensSize) return;
                }
            }
           
            start = (end > (std::string::npos - 1)) ?
                    std::string::npos : end + 1;
        }
    }


    static const std::string trim(const std::string& pString, 
                                  const std::string& pWhitespace = " \t") 
    { 
        const size_t beginStr = pString.find_first_not_of(pWhitespace); 
        if (beginStr == std::string::npos) 
        {
            // No content.
            return ""; 
        }
 
        const size_t endStr = pString.find_last_not_of(pWhitespace); 
        const size_t range  = endStr - beginStr + 1; 
 
        return pString.substr(beginStr, range); 
    } 
 

    static const std::string reduce(const std::string& pString, 
                                    const std::string& pFill = " ", 
                                    const std::string& pWhitespace = " \t") 
    { 
        // Trim first.
        std::string result(trim(pString, pWhitespace)); 
 
        // Replace sub ranges.
        size_t beginSpace = result.find_first_of(pWhitespace); 
        while (beginSpace != std::string::npos) 
        {
            const size_t endSpace = 
                result.find_first_not_of(pWhitespace, beginSpace); 
            const size_t range = endSpace - beginSpace; 
 
            result.replace(beginSpace, range, pFill); 
 
            const size_t newStart = beginSpace + pFill.length(); 
            beginSpace = result.find_first_of(pWhitespace, newStart); 
        } 
 
        return result; 
    } 


    static const std::string getfilename(std::string nameWithPath)
    {
        int i = nameWithPath.length() - 1;
        for (; i >= 0; --i)
        {
            if ((nameWithPath[i] == '/') || (nameWithPath[i] == '\\'))
            {
                break;
            }
        }

        return nameWithPath.substr(i + 1);
    }


    static const std::string substitute(std::string instr, 
                                        const std::string pattern,
                                        const std::string target)
    {
        if (pattern != target)
        {
            size_t found;
            while ((found = instr.find(pattern)) != std::string::npos)
            {
                instr.replace(found, pattern.length(), target);
            }
        }

        return instr;
    }


    static void replace(std::string& str,
                        const std::string& oldStr,
                        const std::string& newStr)
    {
        size_t pos = 0;
        while ((pos = str.find(oldStr, pos)) != std::string::npos)
        {
            str.replace(pos, oldStr.length(), newStr);
            pos += newStr.length();
        }
    }


    template <typename T> static void string2number(const std::string& str, T& num)
    {
        std:: stringstream ss;
        ss << str;
        SInt64 tmp;
        ss >> tmp;
        num = (T)tmp;
    }


    static void string2double(const std::string& str,
                              Double&            num)
    {
        std:: stringstream ss;
        ss << str;
        ss >> num;
    }


    static void string2float(const std::string& str,
                             Float&             num)
    {
        std:: stringstream ss;
        ss << str;
        ss >> num;
    }


    template <typename T> static std::string printVec(const std::vector<T>& vec)
    {
        std::ostringstream os;
        os << "[ ";

        for (typename std::vector<T>::const_iterator it = vec.begin(); it != vec.end(); ++it)
        {
            os << *it << " ";
        }
        os << "]";

        return os.str();
    }


    static void capitalizeFirst(std::string& str)
    {
        str[0] = (char)std::toupper(str[0]);
        for (size_t i = 2; i < str.length(); ++i) {
            if (std::isspace(str[i - 1]) && std::isalpha(str[i])) {
                str[i] = (char)std::toupper(str[i]);
                ++i;
            }
        }
    }

};  // Tokenizer


}  // Close util namespace
}  // Close BoriSoft namespace


#endif  // _TOKENIZER_H_
