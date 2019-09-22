
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <sys/poll.h>

#include "common.hpp"

static
void transact( int client_fd, int serial_fd, int wait_time ) {
	usleep(wait_time);

	for (;;) {
		struct pollfd pfd[2];
		pfd[0].fd = client_fd;
		pfd[0].events = POLLIN | POLLHUP | POLLERR;
		pfd[0].revents = 0;
		pfd[1].fd = serial_fd;
		pfd[1].events = POLLIN | POLLHUP | POLLERR;
		pfd[1].revents = 0;
		if ( poll( pfd, 2, -1 ) < 0 ) {
			fprintf( stderr, "Poll error? [%s]\n", strerror(errno) );
		} else {
			unsigned char buffer[1024];
			int len;

			if ( pfd[0].revents & POLLIN ) {
				len = read( client_fd, buffer, sizeof(buffer) );
				if ( len == 0 ) break;	// client closed
				if ( len < 0 ) {
					fprintf( stderr, "client read error [%s]\n", strerror(errno) );
					break;
				}

				if ( write( serial_fd, buffer, len ) != len ) {
					fprintf( stderr, "serial write problemette\n" );
				}
			}
			if ( pfd[0].revents & POLLERR ) {
				printf("client error\n");
			}
			if ( pfd[0].revents & POLLHUP ) {
				printf("client hungup\n");
			}
			if ( pfd[1].revents & POLLIN ) {
				printf("serial pollin\n");
				len = read( serial_fd, buffer, sizeof(buffer) );
				if ( len < 0 ) {
					fprintf( stderr, "serial read error [%s]\n", strerror(errno) );
					break;
				}

				if ( write( client_fd, buffer, len ) != len ) {
					fprintf( stderr, "client write problemette\n" );
				}
			}
			if ( pfd[1].revents & POLLERR ) {
				printf("serial error\n");
			}
			if ( pfd[1].revents & POLLHUP ) {
				printf("serial hungup\n");
			}
		}
	}

	usleep(wait_time);
}

int main( int argc, char** argv ) {
	int rc = 0;

	if ( argc < 2 ) { 
		fprintf( stderr, "%s <ttydevicepath>\n", argv[0] );
		return 1;
	}

	const char* device = argv[1];
	speed_t	baud		=	B115200;
	unsigned int port	=	32700;
	struct termios	term;
	int wait_time = 500;

	// FIXME : parse arguments to change config etc.
	// FIXME : probably want configuration for data and stop size, and parity - for now assuming default of 8,N,1
	//  	   as being what the adapter gives us and what we need :-)

	int m_serial_fd;
	int m_server_fd;

	m_serial_fd = ::open( device, O_RDWR | O_NDELAY | O_NOCTTY );
	if ( m_serial_fd < 0 ) {
		fprintf( stderr, "Failed to open %s [%s]\n", device, strerror(errno) );
		rc = 1;
	} else {
		if ( tcgetattr( m_serial_fd, &term ) < 0 ) {
			fprintf( stderr, "Failed to get serial config : %s\n", strerror(errno) );
			rc = 2;
		} else {
			int opt_dtr = TIOCM_DTR;
			int opt_rts = TIOCM_RTS;

			cfmakeraw( &term );
			cfsetispeed( &term, baud );
			cfsetospeed( &term, baud );
			if ( tcsetattr( m_serial_fd, TCSANOW, &term ) < 0 ) {
				fprintf( stderr, "Failed to set serial config ; %s\n", strerror(errno) );
				rc = 3;
			} else if ( ::ioctl( m_serial_fd, TIOCMBIS, &opt_dtr ) < 0 ) {
				fprintf( stderr, "Failed to set DTR : %s\n", strerror(errno) );
				rc = 4;
			} else if ( ::ioctl( m_serial_fd, TIOCMBIS, &opt_rts ) < 0 ) {
				fprintf( stderr, "Failed to set RTS : %s\n", strerror(errno) );
				rc = 4;
			} else if ( ::tcflush( m_serial_fd, TCIFLUSH ) < 0 ) {
				fprintf( stderr, "Failed iflush : %s\n", strerror(errno) );
			} else {
				m_server_fd = createTCPServerSocket( port );
				if ( m_server_fd < 0 ) {
					fprintf( stderr, "Failed to create server socket [%s]\n", strerror(errno) );
					rc = 5;
				} else {
					while(1) {		// No way or need to quit atm

						printf("Server Waiting for client...\n");
						struct sockaddr_in	sai;
						socklen_t sai_len = sizeof(sai);
						memset( &sai, 0, sizeof(sai) );
						int client_fd = accept( m_server_fd, (struct sockaddr*)&sai, &sai_len );
						if ( client_fd < 0 ) {
							fprintf( stderr, "accept() failed [%s]\n", strerror(errno) );
						}  else {
							printf("  Client %d %08X\n", client_fd, ntohl( sai.sin_addr.s_addr ) );

							if ( ::tcflush( m_serial_fd, TCIFLUSH ) < 0 ) {
								fprintf( stderr, "Failed iflush : %s\n", strerror(errno) );
								// We won't quit - just means the client may get some unexpected responses for a bit. shrug.
							}

							transact( client_fd, m_serial_fd, wait_time );

							printf("  Client close\n");
							close( client_fd );
						}
					}
					close( m_server_fd );
				}
			}
		}
		(void)::close( m_serial_fd );
	}
	return rc;
}

