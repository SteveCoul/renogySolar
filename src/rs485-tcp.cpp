
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>

#include "Common.hpp"
#include "SerialPort.hpp"

///
/// \class RS485TCP
///
/// \brief	Task to expose a UART RS485 Channel to a TCP socket.
///
class RS485TCP  {
public:

	///
	/// \brief	Application entry point.
	///
	/// Open and configure UART from device path supplied on command line, create the TCP server socket,
	/// listen for connections, and each connection will handle a single MODBUS command:response pair
	/// via SerialPort::transact().
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
		int baud = args->getOptionAsInt( "b" );
		unsigned int port	=	args->getOptionAsUnsignedInt( "p" );
		int wait_time = args->getOptionAsInt( "w" );

		int m_server_fd;

		SerialPort serial( device, baud );
		if ( serial.open() >= 0 ) {	

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

						usleep(wait_time);
						(void)serial.transact( client_fd, -1 );
						usleep(wait_time);

						log( LOG_DEBUG, "client closed" );
						close( client_fd );
					}
				}
				close( m_server_fd );
			}
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


