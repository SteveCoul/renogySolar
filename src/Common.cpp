
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/poll.h>
#include <sys/signal.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#include <algorithm>
#include <cctype>

#include "Args.hpp"
#include "Common.hpp"
#include "Log.hpp"

static int should_quit = 0;
int Common::shouldQuit( void ) {
    return should_quit;
}

unsigned long long Common::NOWms( void ) {
    struct timeval tv;
    unsigned long long rc;
    (void)gettimeofday( &tv, NULL );

    rc = tv.tv_sec * 1000;
    rc = rc + ( tv.tv_usec / 1000 );
    return rc;
}

int Common::tcpAccept( int server ) {
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
            rc = accept( server, (struct sockaddr*)&sai, &sai_len );
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

int Common::createTCPServerSocket( unsigned short port ) {
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

void Common::waitForTCPHangup( int fd ) {
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

int Common::connectTCP( const char* ip, unsigned short port ) {
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
        if ( connect( fd, (const sockaddr*)&sai, sizeof(sai) ) < 0 ) {
            log( LOG_ERR, "socket failed to connect %s:%d [%s]", ip, port, strerror(errno) );
            close( fd );
            fd = -1;
        }
    }
    return fd;
}

static
void quit_handler( int sig ) {
    log( LOG_NOTICE, "SIGINT" );
    should_quit = 1;
}

int main( int argc, char** argv ) {
    int rc = 0;
    pid_t pid;
    Args args;
    rc = args.process( argc, argv, defaultargs() );
    if ( rc < 0 ) return rc;

reboot:
    if ( args.getOptionAsBoolean("single") ) {
        pid = (pid_t)0;
        log( LOG_NOTICE, "Running single process mode, no bootstrap" ); // not printed, log not option yet
    } else {
        pid = fork();
    }
    if ( pid == (pid_t)-1 ) {
        log( LOG_CRIT, "Failed to fork on start" );
        rc = -1;
    } else if ( pid == (pid_t) 0 ) {
        signal( SIGINT, quit_handler );
        signal( SIGPIPE, SIG_IGN );
        Log::configureLogging();
        log( LOG_NOTICE, "Started" );
        rc = getclass()(&args);
    } else {
        int finished = 0;
        Log::configureLogging();
        pid = wait( &rc );
        if ( pid == (pid_t)-1 ) {
            log( LOG_CRIT, "wait() failed. should consider a reboot" );
        } else if ( WIFEXITED( rc ) ) {
            log( LOG_NOTICE, "Child exited normally with %d", WEXITSTATUS( rc ) );
            rc = WEXITSTATUS( rc );
            if ( rc == 0 ) {
                finished = 1;
            } else {
                sleep(5);
            }
        } else if ( WIFSIGNALED( rc ) ) {
            log( LOG_NOTICE, "Child exited with signal %d", WTERMSIG( rc ) );
            if ( WCOREDUMP( rc ) ) {
                log( LOG_NOTICE, "Coredump produced" );
            }
        } else if ( WIFSTOPPED( rc ) ) {
            log( LOG_NOTICE, "Child stopped with %d", WSTOPSIG( rc ) );
        } else {
            log( LOG_NOTICE, "Child complete for unknown reason" );
        }

        if (!finished)
            goto reboot;
    }

    return rc;
}

int Common::readLine( int fd, std::string& result ) {
    result = "";
    for (;;) {
        char c;
        int ret = read( fd, &c, 1 );
        if ( ret == 0 ) 
            break;
        else if ( ret < 0 ) {
            log( LOG_WARNING, "error in read() [%s]", strerror(errno) );
            return -1;
        }

        if ( c == '\r' ) {
            /* ignore */
        } else if ( c == '\n' ) {
            break;
        } else {
            result += c;
        }
    }
    return result.length();
}

std::vector<std::string> Common::tokenizeString( std::string source, std::string dividers ) {
    std::vector<std::string> tokens;
    for (;;) {
        size_t where;
        while ( ( where = source.find_first_of( dividers ) ) == 0 ) 
            source.erase(0,1);
        if ( source.empty() ) break;
        if ( where == std::string::npos ) {
            tokens.push_back( source );
            break;
        }
        tokens.push_back( source.substr( 0, where ) );
        source.erase( 0, where );
    }
    return tokens;
}

std::string Common::toUpper( std::string source ) {
    std::string ret;
    std::for_each( source.begin(), source.end(), [&]( char const &c ) {
        ret+=std::toupper( c );
    } );
    return ret;
}

std::string Common::loadTextFile( std::string path ) {
    std::string r;
    int fd = open( path.c_str(), O_RDONLY );
    if ( fd >= 0 ) {
        char buffer[1024];
        size_t len;
        while ( (len=read(fd,buffer,sizeof(buffer))) > 0 ) {
            r.append( buffer, len );
        }
        close( fd );
    }
    return r;
}

