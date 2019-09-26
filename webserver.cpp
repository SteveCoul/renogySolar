
/* worlds stupidest and most dangerous webserver */

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <malloc.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <sys/poll.h>

#include "common.hpp"

int main( int argc, char** argv ) {

	// FIXME proper arg parsing
	int rc;
	int m_server_fd;
	int client_fd;
	int myport = atoi(argv[1]);

	m_server_fd = createTCPServerSocket( myport );
	if ( m_server_fd < 0 ) {
		fprintf( stderr, "Failed to create server socket [%s]\n", strerror(errno) );
		rc = 5;
	} else {
		while(1) {		// No way or need to quit atm
			int client_fd = tcpAccept( m_server_fd );
			if ( client_fd < 0 ) {
				fprintf( stderr, "accept() failed [%s]\n", strerror(errno) );
			} else {
				char buffer[65536];
				int i = 0;
				for (;;) {
					(void)read( client_fd, buffer+i, 1 );
					if ( buffer[i] == '\n' ) break;
					i++;
				}
				buffer[i] = 0;

				i = 0;
				while( ( buffer[i] != ' ' ) && ( buffer[i] != '\t' ) ) i++;
				while( ( buffer[i] == ' ' ) || ( buffer[i] == '\t' ) ) i++;
				while( buffer[i] == '/' ) i++;
	
				char *name = buffer+i;
				
				while( ( buffer[i] != ' ' ) && ( buffer[i] != '\t' ) && ( buffer[i] != '?' ) ) i++;
				buffer[i] = 0;

				if ( name[0] == 0 ) name = "index.html";

				int fd = open( name, O_RDONLY );
				if ( fd < 0 ) {
					sprintf( buffer, "HTTP/1.0 404 Not Found\r\nContent-Length: 0\r\n\r\n" );
					write( client_fd, buffer, strlen(buffer) );
				} else {
					sprintf( buffer, "HTTP/1.0 200 Okay\r\nAccess-Control-Allow-Origin: *\r\nContent-Length: %d\r\n\r\n", lseek( fd, 0, SEEK_END ) );
					lseek( fd, 0, SEEK_SET);
					write( client_fd, buffer, strlen(buffer) );
					for (;;) {
						int d = read( fd, buffer, sizeof(buffer) );
						if ( d == 0 ) break;
						write( client_fd, buffer, d );
						if ( d != sizeof(buffer) ) break;
					}
					close( fd );

					waitForTCPHangup( client_fd );

					close( client_fd );
				}	
			}
		}
	}
}
