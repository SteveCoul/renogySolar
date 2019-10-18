#ifndef __common_hpp__
#define __common_hpp__

#include <syslog.h>

class Common {
public:
	static void log( int level, const char* fmt, ... );
	static int mprintf( char** pp, const char* fmt, ... );
	static int tcpAccept( int server );
	static int createTCPServerSocket( unsigned short port );
	static void waitForTCPHangup( int fd );
	static int connectTCP( const char* ip, unsigned short port );
	static char* mReadLine( int fd );
	static int timedRead( int fd, void* buffer, size_t length, int timeout_ms );
};

typedef int (*bootstrap)( int argc, char** argv );
bootstrap getclass( void );
#define ENTRYPOINT( app ) bootstrap getclass( void ) { return app::main; }

#define log Common::log

#endif

