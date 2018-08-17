/**********************************************************************

Filename    :   BTypes.h
Content     :   

Created     :   June 15, 2009
Authors     :   Boris Rayskiy

Copyright   :   (c) 2009

**********************************************************************/

#ifndef _B_TYPES_H_
#define _B_TYPES_H_


#if defined(_WINDOWS) || defined(WIN32) || defined(WIN64)
	#define BOS_WINDOWS
#elif defined(__CYGWIN__)
	#define BOS_CYGWIN
#elif defined(__APPLE__)
    #define BOS_OSX
#else
	#define BOS_LINUX
#endif


#ifdef BOS_WINDOWS
#include <windows.h>
#else
#include <sys/types.h>
#endif

#include <stdint.h> 
#include <string>
#include <vector>
#include <map>


namespace BoriSoft {


typedef unsigned char       UChar;
typedef UChar               UInt8;
typedef UInt8               UByte;
typedef unsigned short      UInt16;
typedef unsigned int        UInt32;
typedef UInt32              UInt;
typedef unsigned long long  UInt64;
typedef size_t              UPInt;

typedef char                Char;
typedef Char                Byte;
typedef signed char         SInt8;
typedef SInt8               SByte;
typedef short int           Int16;
typedef Int16               SInt16;
typedef int                 Int32;
typedef Int32               SInt32;
typedef SInt32              SInt;
typedef long long           SInt64;

typedef float               Float;
typedef double              Double;
typedef Double              Float32;
typedef long double         Float64;

// Pointer-size integer
typedef size_t              UPInt;
typedef UInt64              SPInt;

// STRING AND VECTOR

typedef std::string          Cell;
typedef std::vector<Cell>    Column;
typedef std::map<Cell, Cell> CellMap;

// MACRO DEFINITIONS

#ifdef BOS_WINDOWS
#define BINLINE             __forceinline
#else
#define BINLINE inline
#endif

#ifdef BOS_WINDOWS
#define BASM _asm
#else
#define BASM asm
#endif

#ifdef BOS_WINDOWS
#define BUNUSED(a)          (a)
#else
#define BUNUSED(a) do {__typeof__ (&a) __attribute__ ((unused)) __tmp = &a; } while(0)
#endif

#ifndef NULL
#define NULL 0
#endif


} // namespace BoriSoft


#endif // _B_TYPES_H_
