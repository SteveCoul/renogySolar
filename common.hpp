#ifndef __common_hpp__
#define __common_hpp__

int mprintf( char** pp, const char* fmt, ... );
int tcpAccept( int server );
int createTCPServerSocket( unsigned short port );
void waitForTCPHangup( int fd );
int connectTCP( const char* ip, unsigned short port );

#endif

