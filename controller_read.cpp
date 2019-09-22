
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

typedef struct {
	unsigned int	raw;
	unsigned int	rawHI;
	unsigned int	rawLO;
	unsigned int	scale;
	bool			asFlag;
	float			asFloat;
	unsigned int	asInteger;
} data_value_t;

static
int openAndConnect( const char* ip, unsigned short port ) {
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

static
int writeComplete( int fd, unsigned char* buffer, int len ) {
	return write( fd, buffer, len ); //FIXME
}

static
int readComplete( int fd, unsigned char* buffer, int len ) {
	return read( fd, buffer, len ); //FIXME
}

static
int transact( const char* ip, unsigned short port, unsigned char* buffer, int send_len, int rx_len ) {
	int fd = openAndConnect( ip, port );
	int rc = -1;
	if ( fd >= 0 ) {
//		printf("> "); for ( int i = 0; i < send_len; i++ ) printf("%02X ", buffer[i] ); printf("\n");
		if ( writeComplete( fd, buffer, send_len ) >= 0 ) {
			if ( readComplete( fd, buffer, 3 ) >= 0 ) {
				if ( buffer[1] & 0x80 ) {
					printf("Exception %d\n", buffer[2] );
					assert(0);	// FIXME read exception
				} else {
					if ( readComplete( fd, buffer+3, rx_len-3 ) >= 0 ) {
//						printf("< "); for ( int i = 0; i < rx_len; i++ ) printf("%02X ", buffer[i] ); printf("\n");
						rc = rx_len;
					}
				}
			}
		}
		close( fd );
	}
	return rc;
}

static
unsigned int calc_crc( unsigned char* buffer, int len ) {
	unsigned int rc = 0xFFFF;

	for ( int i = 0; i < len; i++ ) {
		rc = rc ^ buffer[i];
		for ( int j = 0; j < 8; j++ ) {
			if ( rc & 1 ) {
				rc >>= 1;
				rc = rc ^ 0xA001;
			} else {
				rc >>= 1;
			}
		}
	}
	rc = ( ( rc >> 8 ) & 0xFF ) | ( ( rc << 8 ) & 0xFF00 );
	return rc;
}

#define VT_MASK 0xFF000000
enum {
	VT_OUTPUT_COIL		=	0x01000000,
	VT_INPUT_CONTACT	=	0x02000000,
	VT_INPUT_REGISTER	=	0x03000000,
	VT_OUTPUT_REGISTER	=	0x04000000,
	VT_UNKNOWN			=	0xFF000000
};

static
void setValues( int scale, int count, data_value_t* p, unsigned char* stream ) {
	for ( int i = 0; i < count; i++ ) {
		p->rawHI = *stream++;
		p->rawLO = *stream++;
		p->scale = scale;
		p->raw = ( p->rawHI << 8 ) | p->rawLO;
		p->asInteger = p->raw / scale;
		p->asFlag = ( p->asInteger != 0 );
		float f = (float)p->raw;
		f = f / scale;
		p->asFloat = f;
		p++;
	}
}

static
int writeRawVariable( const char* ip, unsigned short port, unsigned int unit, unsigned int address, unsigned int count, data_value_t* data ) {
	unsigned char buffer[256];
	int len = 0;
	unsigned int crc;
	int rc;

	if ( ( address & VT_MASK ) == VT_OUTPUT_COIL ) {
		assert(0);
	} else if ( ( address & VT_MASK ) == VT_INPUT_CONTACT ) {
		assert(0);
	} else if ( ( address & VT_MASK ) == VT_INPUT_REGISTER ) {

		address &= ~VT_MASK;

		buffer[len++] = unit;
		buffer[len++] = 0x10;
		buffer[len++] = ( (address) >> 8 ) & 0xFF;
		buffer[len++] = ( (address)      ) & 0xFF;
		buffer[len++] = ( (count) >> 8 ) & 0xFF;
		buffer[len++] = ( (count) ) & 0xFF;
		buffer[len++] = count * 2;
		for ( int i = 0; i < count; i++ ) {
			buffer[len++] = data[i].rawHI;
			buffer[len++] = data[i].rawLO;
		}
		crc = calc_crc( buffer, len );
		buffer[len++] = ( crc >> 8 ) & 0xFF;
		buffer[len++] = ( crc ) & 0xFF;
		len = transact( ip, port, buffer, len, 8 );

		// FIXME verify return
		rc = 0;
	} else if ( ( address & VT_MASK ) == VT_OUTPUT_REGISTER ) {
		assert(0);
	} else {
		rc = -1;
	}
	return rc;
}


static
int readVariable( const char* ip, unsigned short port, unsigned int unit, unsigned int address, unsigned int scale, unsigned int count, data_value_t* p_result ) {
	unsigned char buffer[256];
	int len = 0;
	unsigned int crc;
	int rc;

	if ( ( address & VT_MASK ) == VT_OUTPUT_COIL ) {
		assert(0);
	} else if ( ( address & VT_MASK ) == VT_INPUT_CONTACT ) {

		address &= ~VT_MASK;

		buffer[0] = unit;
		buffer[1] = 0x04;
		buffer[2] = ( (address) >> 8 ) & 0xFF;
		buffer[3] = ( (address)      ) & 0xFF;
		buffer[4] = ( (count) >> 8 ) & 0xFF;
		buffer[5] = ( (count) ) & 0xFF;
		crc = calc_crc( buffer, 6 );
		buffer[6] = ( crc >> 8 ) & 0xFF;
		buffer[7] = ( crc ) & 0xFF;
		len = 8;
		len = transact( ip, port, buffer, len, 5+(count*2) );

		// FIXME verify return

		setValues( scale, count, p_result, buffer+3 );
		rc = 0;
	} else if ( ( address & VT_MASK ) == VT_INPUT_REGISTER ) {

		address &= ~VT_MASK;

		buffer[0] = unit;
		buffer[1] = 0x03;
		buffer[2] = ( (address) >> 8 ) & 0xFF;
		buffer[3] = ( (address)      ) & 0xFF;
		buffer[4] = ( (count) >> 8 ) & 0xFF;
		buffer[5] = ( (count) ) & 0xFF;
		crc = calc_crc( buffer, 6 );
		buffer[6] = ( crc >> 8 ) & 0xFF;
		buffer[7] = ( crc ) & 0xFF;
		len = 8;
		len = transact( ip, port, buffer, len, 5+(count*2) );

		// FIXME verify return

		setValues( scale, count, p_result, buffer+3 );
		rc = 0;
	} else if ( ( address & VT_MASK ) == VT_OUTPUT_REGISTER ) {
		assert(0);
	} else {
		rc = -1;
	}
	return rc;
}

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
				data_value_t value[3];

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

				(void)readVariable( rs485addr, rs485port, id, VT_INPUT_REGISTER | 0x9013, 1, 3, value ); 

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
			
					(void)writeRawVariable( rs485addr, rs485port, id, VT_INPUT_REGISTER | 0x9013, 3, value );
				}

				unsigned int tmp;
				char buffer[65536];
				char* p;

				memset( buffer, 0, sizeof(buffer) );

				p = buffer;

				p+=sprintf(p,"<controller id=\"%d\">\n", id );

				p+=sprintf(p,"\t<pv_array_rating>\n");
				(void)readVariable( rs485addr, rs485port, id, VT_INPUT_CONTACT | 0x3000, 100, 1, value ); 
				p+=sprintf(p,"\t\t<voltage>%f</voltage>\n", value[0].asFloat );
			
				(void)readVariable( rs485addr, rs485port, id, VT_INPUT_CONTACT | 0x3001, 100, 1, value ); 
				p+=sprintf(p,"\t\t<current>%f</current>\n", value[0].asFloat );
				
				(void)readVariable( rs485addr, rs485port, id, VT_INPUT_CONTACT | 0x3002, 100, 2, value ); 
				tmp = ( value[0].raw << 16 ) | value[1].raw;
				p+=sprintf(p,"\t\t<power>%f</power>\n", value[0].asFloat );
				p+=sprintf(p,"\t</pv_array_rating>\n");
				
				p+=sprintf(p,"\t<pv_array_now>\n");
				(void)readVariable( rs485addr, rs485port, id, VT_INPUT_CONTACT | 0x3100, 100, 1, value ); 
				p+=sprintf(p,"\t\t<voltage>%f</voltage>\n", value[0].asFloat );
				
				(void)readVariable( rs485addr, rs485port, id, VT_INPUT_CONTACT | 0x3101, 100, 1, value ); 
				p+=sprintf(p,"\t\t<current>%f</current>\n", value[0].asFloat );

				(void)readVariable( rs485addr, rs485port, id, VT_INPUT_CONTACT | 0x3102, 100, 2, value ); 
				tmp = ( value[0].raw << 16 ) | value[1].raw;
				p+=sprintf(p,"\t\t<power>%f</power>\n", value[0].asFloat );
				p+=sprintf(p,"\t</pv_array_now>\n");
				
				p+=sprintf(p,"\t<battery>\n");
				(void)readVariable( rs485addr, rs485port, id, VT_INPUT_CONTACT | 0x3104, 100, 1, value ); 
				p+=sprintf(p,"\t\t<voltage>%f</voltage>\n", value[0].asFloat );

				(void)readVariable( rs485addr, rs485port, id, VT_INPUT_CONTACT | 0x3105, 100, 1, value ); 
				p+=sprintf(p,"\t\t<current>%f</current>\n", value[0].asFloat );

				(void)readVariable( rs485addr, rs485port, id, VT_INPUT_CONTACT | 0x311A, 1, 1, value ); 
				p+=sprintf(p,"\t\t<state_of_charge>%f</state_of_charge>\n", value[0].asFloat );
				p+=sprintf(p,"\t</battery>\n");

				p+=sprintf(p,"\t<generation>\n");
				(void)readVariable( rs485addr, rs485port, id, VT_INPUT_CONTACT | 0x330C, 100, 2, value ); 
				tmp = ( value[1].raw << 16 ) | value[0].raw;
				p+=sprintf(p,"\t\t<today>%f</today>\n", ((float)tmp)/100.0f );

				(void)readVariable( rs485addr, rs485port, id, VT_INPUT_CONTACT | 0x330E, 100, 2, value ); 
				tmp = ( value[1].raw << 16 ) | value[0].raw;
				p+=sprintf(p,"\t\t<this_month>%f</this_month>\n", ((float)tmp)/100.0f );

				(void)readVariable( rs485addr, rs485port, id, VT_INPUT_CONTACT | 0x3310, 100, 2, value ); 
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
			}
		}
	}
}
