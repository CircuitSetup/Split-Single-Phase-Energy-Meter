#ifndef __DEBUG_H
#define __DEBUG_H

//#define DEBUG
//#define DEBUG_PORT Serial

#define TEXTIFY(A) #A
#define ESCAPEQUOTE(A) TEXTIFY(A)

#ifdef ENABLE_DEBUG

#ifndef DEBUG_PORT
#define DEBUG_PORT Serial1
#endif

#define DEBUG_BEGIN(speed)  DEBUG_PORT.begin(speed)

#define DBUGF(format, ...)  DEBUG_PORT.printf(format "\n", ##__VA_ARGS__)
#define DBUG(...)           DEBUG_PORT.print(__VA_ARGS__)
#define DBUGLN(...)         DEBUG_PORT.println(__VA_ARGS__)

#else

#define DEBUG_BEGIN(speed)
#define DBUGF(...)
#define DBUG(...)
#define DBUGLN(...)

#endif // DEBUG

#endif // __DEBUG_H
