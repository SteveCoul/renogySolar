#ifndef __TCP_hpp__
#define __TCP_hpp__

class TCP {
public:
    static int accept( int server );
    static int createServerSocket( unsigned short port );
    static void waitForHangup( int fd );
    static int connect( const char* ip, unsigned short port );
};

#endif

