
#include <errno.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <syslog.h>
#include <unistd.h>
#include <sys/poll.h>
#include <sys/wait.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#include "Common.hpp"

static int LOG_ALERT_enabled = 1;
static int LOG_CRIT_enabled = 1;
static int LOG_ERR_enabled = 1;
static int LOG_WARNING_enabled = 1;
static int LOG_NOTICE_enabled = 1;
static int LOG_INFO_enabled = 0;
static int LOG_DEBUG_enabled = 0;

void Common::log( int level, const char* fmt, ... ) {
	int output = 0;
	switch( level ) {
	case LOG_ALERT:
		output = LOG_ALERT_enabled;
		break;
	case LOG_CRIT:
		output = LOG_CRIT_enabled;
		break;
	case LOG_ERR:
		output = LOG_ERR_enabled;
		break;
	case LOG_WARNING:
		output = LOG_WARNING_enabled;
		break;
	case LOG_NOTICE:
		output = LOG_NOTICE_enabled;
		break;
	case LOG_INFO:
		output = LOG_INFO_enabled;
		break;
	case LOG_DEBUG:
		output = LOG_DEBUG_enabled;
		break;
	default:
		output = 0;
		break;
	}

	if ( output ) {
		va_list args;
		va_start( args, fmt );
		vsyslog( level, fmt, args );
		va_end( args );
	}
}

int Common::mprintf( char** pp, const char* fmt, ... ) {
	char* n = (char*)malloc(0);
	va_list args;

	va_start( args, fmt );
	int nl = vsnprintf( n, 0, fmt, args );
	va_end( args );

	int ol = 0;
	if ( pp[0] != NULL ) ol = strlen( pp[0] );

	// FIX potential memory leak and warn to syslog on error please.

	pp[0] = (char*)realloc( pp[0], ol + nl + 1024 );
	va_start( args, fmt );
	int rc = vsprintf( pp[0] + ol, fmt, args );
	va_end( args );
	return rc;
}

int Common::tcpAccept( int server ) {
	struct sockaddr_in sai;
	socklen_t sai_len = sizeof(sai);
	memset( &sai, 0, sizeof(sai) );
	return accept( server, (struct sockaddr*)&sai, &sai_len );
}

int Common::createTCPServerSocket( unsigned short port ) {
	int server_fd;

	server_fd = socket( AF_INET, SOCK_STREAM, IPPROTO_TCP );
	if ( server_fd < 0 ) {
		fprintf( stderr, "Failed to create server socket [%s]\n", strerror(errno) );
	} else {
		int opt = 1;
		if ( setsockopt( server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(int) ) < 0 ) {
			fprintf( stderr, "Failed to set server socket reuseaddr [%s]\n", strerror(errno) );
			close( server_fd );
			server_fd = -1;
		} else {
			struct sockaddr_in	sai;
			memset( &sai, 0, sizeof(sai) );
			sai.sin_family = AF_INET;
			sai.sin_port = htons( port );
			sai.sin_addr.s_addr = htonl( INADDR_ANY );
			if ( bind( server_fd, (const sockaddr*)&sai, sizeof(sai) ) < 0 ) {
				fprintf( stderr, "Failed to bind server socket [%s]\n", strerror(errno) );
				close( server_fd );
				server_fd = -1;
			} else if ( listen( server_fd, 1 ) < 0 ) {
				fprintf( stderr, "Server wont listen [%s]\n", strerror(errno) );
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

		int ret = poll( &pfd, 1, -1 );
	
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
		fprintf( stderr, "Failed to create server socket [%s]\n", strerror(errno) );
	} else {
		struct sockaddr_in	sai;
		memset( &sai, 0, sizeof(sai) );
		sai.sin_family = AF_INET;
		sai.sin_port = htons( port );
		sai.sin_addr.s_addr = inet_addr(ip);
		if ( connect( fd, (const sockaddr*)&sai, sizeof(sai) ) < 0 ) {
			fprintf( stderr, "Failed to connect [%s]\n", strerror(errno) );
			close( fd );
			fd = -1;
		}
	}
	return fd;
}

char* Common::mReadLine( int fd ) {			// FIXME
	char* rc = (char*)malloc(65536);
	int i = 0;
	for (;;) {
		int ret = read( fd, rc+i, 65536-i );
		if ( ret < 0 ) {
			free( (void*)rc );
			rc = NULL;
			log( LOG_WARNING, "failed read" );
			break;
		}

		if ( ret == 0 ) { /* remote closed */
			break;
		}
		for ( int j = i; j < i+ret; j++ ) {
			if ( ( rc[j] == '\r' ) || ( rc[j] == '\n' ) ) {
				rc[j] = '\0';
				return rc;
			}
		}
	}
	return rc;
}

int main( int argc, char** argv ) {
	int rc = 0;
	pid_t pid;

reboot:
	pid = fork();
	if ( pid == (pid_t)-1 ) {
		log( LOG_CRIT, "Failed to fork on start" );
		rc = -1;
	} else if ( pid == (pid_t) 0 ) {
		openlog( NULL, LOG_PID, LOG_USER );
		log( LOG_NOTICE, "Started" );
		getclass()( argc, argv );
		rc = 0;
	} else {
		openlog( NULL, LOG_PID, LOG_USER );
		pid = wait( &rc );
		if ( pid == (pid_t)-1 ) {
			log( LOG_CRIT, "wait() failed. should consider a reboot" );
			rc = 1;
		} else if ( WIFEXITED( rc ) ) {
			log( LOG_NOTICE, "Child exited normally with %d", WEXITSTATUS( rc ) );
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

		goto reboot;
	}

	return rc;
}

