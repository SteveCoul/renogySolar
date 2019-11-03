
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <termios.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <sys/poll.h>

#include "Common.hpp"

///
/// \class RS485TCP
///
/// \brief	Task to expose a UART RS485 Channel to a TCP socket.
///
class RS485TCP  {
public:
	///
	/// \brief	This routine will perform a single MODBUS transaction between a TCP socket and a UART.
	///
	/// It will transfer all it can read from the client until it closes transferring content
	/// to the serial port, and any incoming serial data is sent to the client. 
	///
	/// A suitable delay is enforced on the start and end of the transaction as required buy
	/// MODBUS protocol ( 3.5 character time ).
	///
	/// \param	client_fd		TCP socket acting for client.
	/// \param 	serial_fd		Serial port for RS485
	/// \param	wait_time		uSecond delay that should represent 3.5 character time at 
	///							the selected UART baud rate.
	///
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
				log( LOG_WARNING, "Poll error? [%s]\n", strerror(errno) );
			} else {
				unsigned char buffer[1024];
				int len;
	
				if ( pfd[0].revents & POLLIN ) {
					len = read( client_fd, buffer, sizeof(buffer) );
					if ( len == 0 ) break;	// client closed
					if ( len < 0 ) {
						log( LOG_WARNING, "client read error [%s]\n", strerror(errno) );
						break;
					}

					char* dbg = NULL;
					Common::mprintf(&dbg, "< ");
					for ( int i = 0; i < len; i++ ) Common::mprintf(&dbg," %02X", buffer[i] );
					log( LOG_DEBUG, dbg );
					free( (void*)dbg );
	
					if ( write( serial_fd, buffer, len ) != len ) {
						log( LOG_WARNING, "serial write problemette\n" );
					}
				}
				if ( pfd[0].revents & POLLERR ) {
					log( LOG_WARNING, "Client error\n");
				}
				if ( pfd[0].revents & POLLHUP ) {
					log( LOG_WARNING, "client hungup\n");
				}
				if ( pfd[1].revents & POLLIN ) {
					len = read( serial_fd, buffer, sizeof(buffer) );
					if ( len < 0 ) {
						log( LOG_ERR, "serial read error [%s]\n", strerror(errno) );
						break;
					}
	
					char* dbg = NULL;
					Common::mprintf(&dbg, "> ");
					for ( int i = 0; i < len; i++ ) Common::mprintf(&dbg," %02X", buffer[i] );
					log( LOG_DEBUG, dbg );
					free( (void*)dbg );
	
					if ( write( client_fd, buffer, len ) != len ) {
						log( LOG_WARNING, "client write problemette\n" );
					}
				}
				if ( pfd[1].revents & POLLERR ) {
					log( LOG_WARNING,"serial error\n");
				}
				if ( pfd[1].revents & POLLHUP ) {
					log( LOG_WARNING, "serial hungup\n");
				}
			}
		}
	
		usleep(wait_time);
	}
	
	///
	/// \brief	Application entry point.
	///
	/// Open and configure UART from device path supplied on command line, create the TCP server socket,
	/// listen for connections, and each connection will handle a single MODBUS command:response pair
	/// via transact().
	///
	/// \param args	command arguments
	///
	/// \return app code.
	///
	static
	int main( Args* args ) {
		int rc = 0;
	
		log( LOG_INFO, "Starting" );

		const char* device	=	args->getOptionAsString( "d" );
		speed_t	baud		=	B115200;
		unsigned int port	=	args->getOptionAsUnsignedInt( "p" );
		struct termios	term;
		int wait_time = args->getOptionAsInt( "w" );

		int m_serial_fd;
		int m_server_fd;

		switch( args->getOptionAsInt( "b" ) ) {
		case 115200:
			baud = B115200;
			break;
		default:
			baud = B115200;
			log( LOG_WARNING, "Unknown baud rate, using 115200" );
			break;
		}

		log( LOG_INFO, "Open device %s at baud rate %d, serve on port %d, use wait_time %d", device, args->getOptionAsInt("b"), port, wait_time );

		m_serial_fd = ::open( device, O_RDWR | O_NDELAY | O_NOCTTY );
		if ( m_serial_fd < 0 ) {
			log( LOG_CRIT, "Failed to open %s [%s]\n", device, strerror(errno) );
			rc = 1;
		} else {
			if ( tcgetattr( m_serial_fd, &term ) < 0 ) {
				log( LOG_CRIT, "Failed to get serial config : %s\n", strerror(errno) );
				rc = 2;
			} else {
				int opt_dtr = TIOCM_DTR;
				int opt_rts = TIOCM_RTS;
	
				cfmakeraw( &term );
				cfsetispeed( &term, baud );
				cfsetospeed( &term, baud );
				if ( tcsetattr( m_serial_fd, TCSANOW, &term ) < 0 ) {
					log( LOG_CRIT, "Failed to set serial config ; %s\n", strerror(errno) );
					rc = 3;
				} else if ( ::ioctl( m_serial_fd, TIOCMBIS, &opt_dtr ) < 0 ) {
					log( LOG_CRIT, "Failed to set DTR : %s\n", strerror(errno) );
					rc = 4;
				} else if ( ::ioctl( m_serial_fd, TIOCMBIS, &opt_rts ) < 0 ) {
					log( LOG_CRIT, "Failed to set RTS : %s\n", strerror(errno) );
					rc = 4;
				} else if ( ::tcflush( m_serial_fd, TCIFLUSH ) < 0 ) {
					log( LOG_CRIT, "Failed iflush : %s\n", strerror(errno) );
				} else {
					m_server_fd = Common::createTCPServerSocket( port );
					if ( m_server_fd < 0 ) {
						log( LOG_CRIT, "Failed to create server socket [%s]\n", strerror(errno) );
						rc = 5;
					} else {
						while(1) {		// No way or need to quit atm
	
							log( LOG_DEBUG, "Server waiting for client connect" );
							struct sockaddr_in	sai;
							socklen_t sai_len = sizeof(sai);
							memset( &sai, 0, sizeof(sai) );
							int client_fd = accept( m_server_fd, (struct sockaddr*)&sai, &sai_len );
							if ( client_fd < 0 ) {
								log( LOG_ERR, "accept() failed [%s]\n", strerror(errno) );
							}  else {
								log( LOG_DEBUG,"  Client %d %08X\n", client_fd, ntohl( sai.sin_addr.s_addr ) );

								transact( client_fd, m_serial_fd, wait_time );
	
								log( LOG_DEBUG, "client closed" );
								close( client_fd );
							}
						}
						close( m_server_fd );
					}
				}
			}
			(void)::close( m_serial_fd );
		}
	
		closelog();
		return rc;
	}

};

ENTRYPOINT( RS485TCP )
DEFAULT_ARGS( 	"-d:path to serial port device:/dev/ttyUSB0"
			  	"-p:TCP port to listen on:32700"
				"-b:Baud Rate for Serial Port:115200"
				"-w:Wait time on connect/disconnect:750" )


