#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/poll.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#include <Common.hpp>
#include <Log.hpp>
#include <TCP.hpp>

int TCP::accept( int server ) {
    int rc = -1;
    struct sockaddr_in sai;
    socklen_t sai_len = sizeof(sai);
    memset( &sai, 0, sizeof(sai) );

    struct pollfd pfd;
    pfd.fd = server;
    pfd.events = POLLIN | POLLHUP;
    for (;;) {
        pfd.revents = 0;

        int ret = poll( &pfd, 1, 300 );
  
        if ( ret < 0 ) {
            rc = ret;
            break;
        }
 
        if ( Common::shouldQuit() ) {
            rc = -1;
            errno = EINTR;
            break;
        }
 
        if ( pfd.revents & POLLIN ) {
            rc = ::accept( server, (struct sockaddr*)&sai, &sai_len );
            if ( rc < 0 ) {
                log( LOG_ERR, "Failed to accept tcp connection [%s]", strerror(errno) );
            }
            break;
        }
        if ( pfd.revents & POLLHUP ) {
            break;
        }   
    }
    return rc;
}

int TCP::createServerSocket( unsigned short port ) {
    int server_fd;

    server_fd = socket( AF_INET, SOCK_STREAM, IPPROTO_TCP );
    if ( server_fd < 0 ) {
        log( LOG_ERR, "Failed to create server socket [%s]", strerror(errno) );
    } else {
        int opt = 1;
        if ( setsockopt( server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(int) ) < 0 ) {
            log( LOG_ERR, "Failed to set server socket reuseaddr [%s]", strerror(errno) );
            close( server_fd );
            server_fd = -1;
        } else {
            struct sockaddr_in  sai;
            memset( &sai, 0, sizeof(sai) );
            sai.sin_family = AF_INET;
            sai.sin_port = htons( port );
            sai.sin_addr.s_addr = htonl( INADDR_ANY );
            if ( bind( server_fd, (const sockaddr*)&sai, sizeof(sai) ) < 0 ) {
                log( LOG_ERR, "Failed to bind server socket port %d [%s]", port, strerror(errno) );
                close( server_fd );
                server_fd = -1;
            } else if ( listen( server_fd, 1 ) < 0 ) {
                log( LOG_ERR, "Server wont listen [%s]", strerror(errno) );
                close( server_fd );
                server_fd = -1;
            } else {
                /* cool */
            }
        }
    }
    return server_fd;
}

void TCP::waitForHangup( int fd ) {
    struct pollfd pfd;
    pfd.fd = fd;
    pfd.events = POLLIN | POLLHUP;
    for (;;) {
        pfd.revents = 0;

        int ret = poll( &pfd, 1, 333 );
    
        if ( Common::shouldQuit() ) {
            break;
        }

        if ( pfd.revents & POLLIN ) {
            char buffer[1024];
            ret = read( fd, buffer, sizeof(buffer) );
            if ( ret == 0 ) break;
        }
        if ( pfd.revents & POLLHUP ) {
            break;
        }   
    }
}

int TCP::connect( const char* ip, unsigned short port ) {
    int fd = -1;

    fd = socket( AF_INET, SOCK_STREAM, IPPROTO_TCP );
    if ( fd < 0 ) {
        log( LOG_ERR, "Failed to create socket [%s]", strerror(errno) );
    } else {
        struct sockaddr_in  sai;
        memset( &sai, 0, sizeof(sai) );
        sai.sin_family = AF_INET;
        sai.sin_port = htons( port );
        sai.sin_addr.s_addr = inet_addr(ip);
        if ( ::connect( fd, (const sockaddr*)&sai, sizeof(sai) ) < 0 ) {
            log( LOG_ERR, "socket failed to connect %s:%d [%s]", ip, port, strerror(errno) );
            close( fd );
            fd = -1;
        }
    }
    return fd;
}

