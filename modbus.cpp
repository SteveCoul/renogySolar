#include <assert.h>
#include <errno.h>
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
#include "modbus.hpp"

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
	int fd = Common::connectTCP( ip, port );
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
void setValues( int scale, int count, modbus_data_value_t* p, unsigned char* stream ) {
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

int modbusReadVariable( const char* ip, unsigned short port, unsigned int unit, unsigned int address, unsigned int scale, unsigned int count, modbus_data_value_t* p_result ) {
	unsigned char buffer[256];
	int len = 0;
	unsigned int crc;
	int rc;

	if ( ( address & MODBUS_VT_MASK ) == MODBUS_VT_OUTPUT_COIL ) {
		assert(0);
	} else if ( ( address & MODBUS_VT_MASK ) == MODBUS_VT_INPUT_CONTACT ) {

		address &= ~MODBUS_VT_MASK;

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
	} else if ( ( address & MODBUS_VT_MASK ) == MODBUS_VT_INPUT_REGISTER ) {

		address &= ~MODBUS_VT_MASK;

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
	} else if ( ( address & MODBUS_VT_MASK ) == MODBUS_VT_OUTPUT_REGISTER ) {
		assert(0);
	} else {
		rc = -1;
	}
	return rc;
}

int modbusWriteRawVariable( const char* ip, unsigned short port, unsigned int unit, unsigned int address, unsigned int count, modbus_data_value_t* data ) {
	unsigned char buffer[256];
	int len = 0;
	unsigned int crc;
	int rc;

	if ( ( address & MODBUS_VT_MASK ) == MODBUS_VT_OUTPUT_COIL ) {
		assert(0);
	} else if ( ( address & MODBUS_VT_MASK ) == MODBUS_VT_INPUT_CONTACT ) {
		assert(0);
	} else if ( ( address & MODBUS_VT_MASK ) == MODBUS_VT_INPUT_REGISTER ) {

		address &= ~MODBUS_VT_MASK;

		buffer[len++] = unit;
		buffer[len++] = 0x10;
		buffer[len++] = ( (address) >> 8 ) & 0xFF;
		buffer[len++] = ( (address)      ) & 0xFF;
		buffer[len++] = ( (count) >> 8 ) & 0xFF;
		buffer[len++] = ( (count) ) & 0xFF;
		buffer[len++] = count * 2;
		for ( unsigned int i = 0; i < count; i++ ) {
			buffer[len++] = data[i].rawHI;
			buffer[len++] = data[i].rawLO;
		}
		crc = calc_crc( buffer, len );
		buffer[len++] = ( crc >> 8 ) & 0xFF;
		buffer[len++] = ( crc ) & 0xFF;
		len = transact( ip, port, buffer, len, 8 );

		// FIXME verify return
		rc = 0;
	} else if ( ( address & MODBUS_VT_MASK ) == MODBUS_VT_OUTPUT_REGISTER ) {
		assert(0);
	} else {
		rc = -1;
	}
	return rc;
}


