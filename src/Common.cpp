
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/signal.h>
#include <sys/time.h>
#include <sys/wait.h>

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


