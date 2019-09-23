#ifndef __common_hpp__
#define __common_hpp__

int createTCPServerSocket( unsigned short port );
void waitForTCPHangup( int fd );
int connectTCP( const char* ip, unsigned short port );

#endif

