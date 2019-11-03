
/* worlds stupidest and most dangerous webserver */

#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
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

#include "Common.hpp"

class WebServer {
public:
static
int main( Args* args ) {

	// FIXME proper arg parsing
	int m_server_fd;
	int myport = args->getOptionAsInt( "p" );

	m_server_fd = Common::createTCPServerSocket( myport );
	if ( m_server_fd > 0 ) {
		while(1) {		// No way or need to quit atm
			int client_fd = Common::tcpAccept( m_server_fd );
			if ( client_fd > 0 ) {
				char* line = Common::mReadLine( client_fd );

				int i = 0;

				while( ( line[i] != ' ' ) && ( line[i] != '\t' ) ) i++;
				while( ( line[i] == ' ' ) || ( line[i] == '\t' ) ) i++;
				while( line[i] == '/' ) i++;
	
				char *name = line+i;
				
				while( ( line[i] != ' ' ) && ( line[i] != '\t' ) && ( line[i] != '?' ) ) i++;
				line[i] = 0;

				char buffer[65536];

				sprintf( buffer, "www/%s", name[0] == '\0' ? "index.html" : name );

				int fd = open( buffer, O_RDONLY );
				if ( fd < 0 ) {
					sprintf( buffer, "HTTP/1.0 404 Not Found\r\nContent-Length: 0\r\n\r\n" );
					write( client_fd, buffer, strlen(buffer) );
				} else {
					sprintf( buffer, "HTTP/1.0 200 Okay\r\nAccess-Control-Allow-Origin: *\r\nContent-Length: %d\r\n\r\n", (int) lseek( fd, 0, SEEK_END ) );
					lseek( fd, 0, SEEK_SET);
					write( client_fd, buffer, strlen(buffer) );
					for (;;) {
						int d = read( fd, buffer, sizeof(buffer) );
						if ( d == 0 ) break;
						write( client_fd, buffer, d );
						if ( d != sizeof(buffer) ) break;
					}
					close( fd );

					Common::waitForTCPHangup( client_fd );

					close( client_fd );
				}	
				free((void*)line);
			}
		}
	}
	return 0;
}
};

ENTRYPOINT( WebServer )
DEFAULT_ARGS( "-p:TCP port to listen on:8080" )

