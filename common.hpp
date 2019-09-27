#ifndef __common_hpp__
#define __common_hpp__

void log( int level, const char* fmt, ... );
int mprintf( char** pp, const char* fmt, ... );
int tcpAccept( int server );
int createTCPServerSocket( unsigned short port );
void waitForTCPHangup( int fd );
int connectTCP( const char* ip, unsigned short port );
char* mReadLine( int fd );

#endif

