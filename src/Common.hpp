#ifndef __common_hpp__
#define __common_hpp__

#include <string>
#include <syslog.h>

#include "Args.hpp"

class Common {
public:
    static int shouldQuit( void );
    static unsigned long long NOWms( void );
    static int tcpAccept( int server );
    static int createTCPServerSocket( unsigned short port );
    static void waitForTCPHangup( int fd );
    static int connectTCP( const char* ip, unsigned short port );
    static int readLine( int fd, std::string& result );
    static std::vector<std::string> tokenizeString( std::string source, std::string dividers );
    static std::string toUpper( std::string source );
    static std::string loadTextFile( std::string path );
};

typedef int (*bootstrap)( Args* args );
bootstrap getclass( void );
const char* defaultargs( void );

#define ENTRYPOINT( app ) bootstrap getclass( void ) { return app::main; }
#define DEFAULT_ARGS( text ) const char* defaultargs( void ) { return text; }

#endif

