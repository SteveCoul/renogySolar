#ifndef __Log_hpp__
#define __Log_hpp__

#include <string>

class Log {
public:
    static void log( int level, const char* fmt, ... );
	static void log( int level, std::string str );
	static void configureLogging( void );
};

#define log Log::log

#endif

