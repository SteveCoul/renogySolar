
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <malloc.h>
#include <signal.h>
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
#include "modbus.hpp"


int main( int argc, char** argv ) {

	// FIXME proper arg parsing
	int rc;
	int m_server_fd;
	int client_fd;

	int rs485port = atoi( argv[2] );
	const char* rs485addr = argv[1];
	int id = atoi( argv[3] );
	int myport = atoi(argv[4]);

	signal( SIGPIPE, SIG_IGN );

	m_server_fd = createTCPServerSocket( myport );
	if ( m_server_fd < 0 ) {
		fprintf( stderr, "Failed to create server socket [%s]\n", strerror(errno) );
		rc = 5;
	} else {
		while(1) {		// No way or need to quit atm
			struct sockaddr_in	sai;

			socklen_t sai_len = sizeof(sai);
			memset( &sai, 0, sizeof(sai) );
			int client_fd = accept( m_server_fd, (struct sockaddr*)&sai, &sai_len );
			if ( client_fd < 0 ) {
				fprintf( stderr, "accept() failed [%s]\n", strerror(errno) );
			} else {
				modbus_data_value_t value[3];

				time_t	now;
				struct tm* tm;

				(void)time(&now);
				tm = localtime( &now );

				unsigned int	local_year = tm->tm_year + 1900;
				unsigned int	local_month = tm->tm_mon + 1;
				unsigned int	local_day = tm->tm_mday;
				unsigned int	local_hour = tm->tm_hour;
				unsigned int 	local_minute = tm->tm_min;
				unsigned int	local_seconds = tm->tm_sec;
				int	local_time = ( local_hour * 3600 ) + ( local_minute * 60 ) + local_seconds;

				(void)modbusReadVariable( rs485addr, rs485port, id, MODBUS_VT_INPUT_REGISTER | 0x9013, 1, 3, value ); 

				unsigned int	remote_year = value[2].rawHI + 2000;
				unsigned int	remote_month = value[2].rawLO;
				unsigned int	remote_day = value[1].rawHI;
				unsigned int	remote_hour = value[1].rawLO;
				unsigned int	remote_minute = value[0].rawHI;
				unsigned int	remote_seconds = value[0].rawLO;
				int	remote_time = ( remote_hour * 3600 ) + ( remote_minute * 60 ) + remote_seconds;

				int time_difference = abs( local_time - remote_time );

				if ( ( local_year != remote_year ) ||
					 ( local_month != remote_month ) ||
					 ( local_day != remote_day ) ||
					 ( time_difference > 30 ) ) {

					value[2].rawHI = local_year - 2000;
					value[2].rawLO = local_month;
					value[1].rawHI = local_day;
					value[1].rawLO = local_hour;
					value[0].rawHI = local_minute;
					value[0].rawLO = local_seconds;
				
					value[0].raw = ( value[0].rawHI << 8 ) | value[0].rawLO;
					value[1].raw = ( value[1].rawHI << 8 ) | value[1].rawLO;
					value[2].raw = ( value[2].rawHI << 8 ) | value[2].rawLO;
			
					(void)modbusWriteRawVariable( rs485addr, rs485port, id, MODBUS_VT_INPUT_REGISTER | 0x9013, 3, value );
				}

				unsigned int tmp;
				char buffer[65536];
				char* p;

				memset( buffer, 0, sizeof(buffer) );

				p = buffer;

				p+=sprintf(p,"<controller id=\"%d\">\n", id );

				p+=sprintf(p,"\t<pv_array_rating>\n");
				(void)modbusReadVariable( rs485addr, rs485port, id, MODBUS_VT_INPUT_CONTACT | 0x3000, 100, 1, value ); 
				p+=sprintf(p,"\t\t<voltage>%f</voltage>\n", value[0].asFloat );
			
				(void)modbusReadVariable( rs485addr, rs485port, id, MODBUS_VT_INPUT_CONTACT | 0x3001, 100, 1, value ); 
				p+=sprintf(p,"\t\t<current>%f</current>\n", value[0].asFloat );
				
				(void)modbusReadVariable( rs485addr, rs485port, id, MODBUS_VT_INPUT_CONTACT | 0x3002, 100, 2, value ); 
				tmp = ( value[0].raw << 16 ) | value[1].raw;
				p+=sprintf(p,"\t\t<power>%f</power>\n", value[0].asFloat );
				p+=sprintf(p,"\t</pv_array_rating>\n");
				
				p+=sprintf(p,"\t<pv_array_now>\n");
				(void)modbusReadVariable( rs485addr, rs485port, id, MODBUS_VT_INPUT_CONTACT | 0x3100, 100, 1, value ); 
				p+=sprintf(p,"\t\t<voltage>%f</voltage>\n", value[0].asFloat );
				
				(void)modbusReadVariable( rs485addr, rs485port, id, MODBUS_VT_INPUT_CONTACT | 0x3101, 100, 1, value ); 
				p+=sprintf(p,"\t\t<current>%f</current>\n", value[0].asFloat );

				(void)modbusReadVariable( rs485addr, rs485port, id, MODBUS_VT_INPUT_CONTACT | 0x3102, 100, 2, value ); 
				tmp = ( value[0].raw << 16 ) | value[1].raw;
				p+=sprintf(p,"\t\t<power>%f</power>\n", value[0].asFloat );
				p+=sprintf(p,"\t</pv_array_now>\n");
				
				p+=sprintf(p,"\t<battery>\n");
				(void)modbusReadVariable( rs485addr, rs485port, id, MODBUS_VT_INPUT_CONTACT | 0x3104, 100, 1, value ); 
				p+=sprintf(p,"\t\t<voltage>%f</voltage>\n", value[0].asFloat );

				(void)modbusReadVariable( rs485addr, rs485port, id, MODBUS_VT_INPUT_CONTACT | 0x3105, 100, 1, value ); 
				p+=sprintf(p,"\t\t<current>%f</current>\n", value[0].asFloat );

				(void)modbusReadVariable( rs485addr, rs485port, id, MODBUS_VT_INPUT_CONTACT | 0x311A, 1, 1, value ); 
				p+=sprintf(p,"\t\t<state_of_charge>%f</state_of_charge>\n", value[0].asFloat );
				p+=sprintf(p,"\t</battery>\n");

				p+=sprintf(p,"\t<generation>\n");
				(void)modbusReadVariable( rs485addr, rs485port, id, MODBUS_VT_INPUT_CONTACT | 0x330C, 100, 2, value ); 
				tmp = ( value[1].raw << 16 ) | value[0].raw;
				p+=sprintf(p,"\t\t<today>%f</today>\n", ((float)tmp)/100.0f );

				(void)modbusReadVariable( rs485addr, rs485port, id, MODBUS_VT_INPUT_CONTACT | 0x330E, 100, 2, value ); 
				tmp = ( value[1].raw << 16 ) | value[0].raw;
				p+=sprintf(p,"\t\t<this_month>%f</this_month>\n", ((float)tmp)/100.0f );

				(void)modbusReadVariable( rs485addr, rs485port, id, MODBUS_VT_INPUT_CONTACT | 0x3310, 100, 2, value ); 
				tmp = ( value[1].raw << 16 ) | value[0].raw;
				p+=sprintf(p,"\t\t<this_year>%f</this_year>\n", ((float)tmp)/100.0f );
				p+=sprintf(p,"\t</generation>\n");

				p+=sprintf( p, "</controller>\n");

				int body_len = p-buffer;

				char headers[65536];
				p = headers;
				p+=sprintf(p,"HTTP/1.0 200 OK\r\n");
				p+=sprintf(p,"Access-Control-Allow-Origin: *\r\n");
				p+=sprintf(p,"Content-Length: %d\r\n", body_len );
				p+=sprintf(p,"\r\n");

				write( client_fd, headers, p-headers );
				write( client_fd, buffer, body_len );

				waitForTCPHangup( client_fd );
				close( client_fd );
			}
		}
	}
}
