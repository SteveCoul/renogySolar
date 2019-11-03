
#include <errno.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <syslog.h>
#include <unistd.h>
#include <sys/poll.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#include "Args.hpp"
#include "Common.hpp"

static int LOG_ALERT_enabled = 0;
static int LOG_CRIT_enabled = 0;
static int LOG_ERR_enabled = 0;
static int LOG_WARNING_enabled = 0;
static int LOG_NOTICE_enabled = 0;
static int LOG_INFO_enabled = 0;
static int LOG_DEBUG_enabled = 0;

static
void configureLogging( void ) {
	const char* e = getenv( "RENOGY_DEBUG" );
	if ( e == NULL ) e = "EWN";

	/* These are always on */
	LOG_ALERT_enabled = 1;	
	LOG_CRIT_enabled = 1;

	/* These maybe */
	for ( int i = 0; e[i] != '\0'; i++ ) {
		switch( e[i] ) {
		case 'e':	
		case 'E':
			LOG_ERR_enabled = 1;
			break;
		case 'w':	
		case 'W':
			LOG_WARNING_enabled = 1;
			break;
		case 'n':	
		case 'N':
			LOG_NOTICE_enabled = 1;
			break;
		case 'i':	
		case 'I':
			LOG_INFO_enabled = 1;
			break;
		case 'd':	
		case 'D':
			LOG_DEBUG_enabled = 1;
			break;
		case '*':
			LOG_ERR_enabled = 1;
			LOG_WARNING_enabled = 1;
			LOG_NOTICE_enabled = 1;
			LOG_INFO_enabled = 1;
			LOG_DEBUG_enabled = 1;
			break;
		default:
			/* ignore unknown values */
			break;
		}
	}
}

static
unsigned long long NOW( void ) {
	struct timeval tv;
	unsigned long long rc;
	(void)gettimeofday( &tv, NULL );

	rc = tv.tv_sec * 1000;
	rc = rc + ( tv.tv_usec / 1000 );
	return rc;
}

int Common::timedRead( int fd, void* buffer, size_t length, int timeout_ms ) {
	unsigned char* ptr = (unsigned char*)buffer;
	struct pollfd pfd;
	unsigned long long start_time = NOW();
	int rc = 0;

	log( LOG_DEBUG, "Timed Read %d", length );

	for (;;) {

		if ( rc == (int)length ) {
			break;
		}

		memset( &pfd, 0, sizeof(pfd) );
		pfd.fd = fd;
		pfd.events = POLLIN;

		int to;
		if ( timeout_ms < 0 ) {
			to = -1;
		} else {
			to = NOW() - start_time;
			if ( to > timeout_ms ) {
				errno = ETIMEDOUT;
				rc = -1;
				break;
			}
		}

		(void)poll( &pfd, 1, to );

		if ( pfd.revents & POLLIN ) {
			int to_read = length - rc;
			int ret = read( fd, ptr, to_read );
			if ( ret < 0 ) {
				rc = -1;
				break;
			}
			log( LOG_DEBUG, "Timed Read got %d wanted %d", ret, to_read );

			rc+=ret;
			ptr+=ret; 
		}
	}

	log( LOG_DEBUG, "Timed Read took %llu ms", NOW() - start_time );
	return rc;
}

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
	int rc;
	struct sockaddr_in sai;
	socklen_t sai_len = sizeof(sai);
	memset( &sai, 0, sizeof(sai) );
	rc = accept( server, (struct sockaddr*)&sai, &sai_len );
	if ( rc < 0 ) {
		log( LOG_ERR, "Failed to accept tcp connection [%s]", strerror(errno) );
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
			struct sockaddr_in	sai;
			memset( &sai, 0, sizeof(sai) );
			sai.sin_family = AF_INET;
			sai.sin_port = htons( port );
			sai.sin_addr.s_addr = htonl( INADDR_ANY );
			if ( bind( server_fd, (const sockaddr*)&sai, sizeof(sai) ) < 0 ) {
				log( LOG_ERR, "Failed to bind server socket [%s]", strerror(errno) );
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
		log( LOG_ERR, "Failed to create socket [%s]", strerror(errno) );
	} else {
		struct sockaddr_in	sai;
		memset( &sai, 0, sizeof(sai) );
		sai.sin_family = AF_INET;
		sai.sin_port = htons( port );
		sai.sin_addr.s_addr = inet_addr(ip);
		if ( connect( fd, (const sockaddr*)&sai, sizeof(sai) ) < 0 ) {
			log( LOG_ERR, "socket failed to connect [%s]", strerror(errno) );
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
		openlog( NULL, LOG_PERROR, LOG_USER );
		configureLogging();
		log( LOG_NOTICE, "Started" );
		Args args;
		rc = args.process( argc, argv, defaultargs() );
		if ( rc >= 0 ) {
			getclass()(&args);
			rc = 0;
		}
	} else {
		int finished = 0;
		openlog( NULL, LOG_PERROR, LOG_USER );
		configureLogging();
		pid = wait( &rc );
		if ( pid == (pid_t)-1 ) {
			log( LOG_CRIT, "wait() failed. should consider a reboot" );
		} else if ( WIFEXITED( rc ) ) {
			log( LOG_NOTICE, "Child exited normally with %d", WEXITSTATUS( rc ) );
			rc = WEXITSTATUS( rc );
			finished = 1;
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

