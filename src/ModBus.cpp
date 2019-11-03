#include <assert.h>
#include <errno.h>
#include <math.h>
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
#include "ModBus.hpp"

unsigned int ModBus::calc_crc( unsigned char* buffer, int len ) {
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

int ModBus::writeComplete( int fd, unsigned char* buffer, int len ) {
	return write( fd, buffer, len ); //FIXME
}

int ModBus::readComplete( int fd, unsigned char* buffer, int len ) {
	int rc = Common::timedRead( fd, (void*)buffer, (size_t)len, 300 );	/* todo, make timeout configurable */
	if ( rc < 0 ) {
		log( LOG_WARNING, "modbus transaction reply didn't return [%s]", strerror(errno) );
	}
	return rc;
}

int ModBus::transact( unsigned char* buffer, int send_len, int rx_len ) {
	int fd = Common::connectTCP( m_ip, m_port );
	int rc = -1;
	if ( fd >= 0 ) {
		char* dbg = NULL;
		Common::mprintf(&dbg,"modbus> "); for ( int i = 0; i < send_len; i++ ) Common::mprintf(&dbg, "%02X ", buffer[i] ); log( LOG_DEBUG, dbg ); free((void*)dbg); dbg=NULL;
		if ( writeComplete( fd, buffer, send_len ) >= 0 ) {
			if ( readComplete( fd, buffer, 3 ) >= 0 ) {
				if ( buffer[1] & 0x80 ) {
					printf("Exception %d\n", buffer[2] );
					assert(0);	// FIXME read exception
				} else {
					if ( readComplete( fd, buffer+3, rx_len-3 ) >= 0 ) {
						Common::mprintf(&dbg,"modbus< "); for ( int i = 0; i < rx_len; i++ ) Common::mprintf(&dbg, "%02X ", buffer[i] ); log( LOG_DEBUG, dbg ); free((void*)dbg);
						rc = rx_len;
					}
				}
			}
		}
		close( fd );
	}
	return rc;
}

void ModBus::setValues( int scale, int count, Value* p, unsigned char* stream ) {
	for ( int i = 0; i < count; i++ ) {
		p->set( stream[0], stream[1], scale );
		stream+=2;
		p++;
	}
}

void ModBus::Value::set( unsigned char hi, unsigned char lo, unsigned int scale ) {
	m_rawHI = hi;
	m_rawLO = lo;
	m_scale = scale;
	m_raw = ( m_rawHI << 8 ) | m_rawLO;
	m_asInteger = m_raw / m_scale;
	m_asFlag = ( m_asInteger != 0 );
	float f = (float)m_raw;
	f = f / m_scale;
	m_asFloat = f;
}

void ModBus::setInvalidValues( int scale, int count, Value* p ) {
	for ( int i = 0; i < count; i++ ) {
		p->invalidate();
		p++;
	}
}

void ModBus::Value::invalidate( void ) {
	m_rawHI = 0;
	m_rawLO = 0;
	m_scale = 0;
	m_raw = 0;
	m_asInteger = 0;
	m_asFlag = false;
	m_asFloat = NAN;
}

int ModBus::readVariable( unsigned int address, unsigned int scale, unsigned int count, Value* p_result ) {
	unsigned char buffer[256];
	int len = 0;
	unsigned int crc;
	int rc = -1;

	if ( ( address & ModBus::VT_MASK ) == ModBus::VT_OUTPUT_COIL ) {
		assert(0);
	} else if ( ( address & ModBus::VT_MASK ) == ModBus::VT_INPUT_CONTACT ) {

		address &= ~ModBus::VT_MASK;

		buffer[0] = m_id;
		buffer[1] = 0x04;
		buffer[2] = ( (address) >> 8 ) & 0xFF;
		buffer[3] = ( (address)      ) & 0xFF;
		buffer[4] = ( (count) >> 8 ) & 0xFF;
		buffer[5] = ( (count) ) & 0xFF;
		crc = calc_crc( buffer, 6 );
		buffer[6] = ( crc >> 8 ) & 0xFF;
		buffer[7] = ( crc ) & 0xFF;
		len = 8;
		len = transact( buffer, len, 5+(count*2) );
		if ( len >= 0 ) {
			setValues( scale, count, p_result, buffer+3 );
			rc = 0;
		} else {
			setInvalidValues( scale, count, p_result );
			rc = -1;
		}
	} else if ( ( address & ModBus::VT_MASK ) == ModBus::VT_INPUT_REGISTER ) {

		address &= ~ModBus::VT_MASK;

		buffer[0] = m_id;
		buffer[1] = 0x03;
		buffer[2] = ( (address) >> 8 ) & 0xFF;
		buffer[3] = ( (address)      ) & 0xFF;
		buffer[4] = ( (count) >> 8 ) & 0xFF;
		buffer[5] = ( (count) ) & 0xFF;
		crc = calc_crc( buffer, 6 );
		buffer[6] = ( crc >> 8 ) & 0xFF;
		buffer[7] = ( crc ) & 0xFF;
		len = 8;
		len = transact( buffer, len, 5+(count*2) );
		if ( len >= 0 ) {
			setValues( scale, count, p_result, buffer+3 );
			rc = 0;
		} else {
			setInvalidValues( scale, count, p_result );
			rc = -1;
		}
	} else if ( ( address & ModBus::VT_MASK ) == ModBus::VT_OUTPUT_REGISTER ) {
		assert(0);
	} else {
		rc = -1;
	}
	return rc;
}

int ModBus::writeRawVariable( unsigned int address, unsigned int count, Value* data ) {
	unsigned char buffer[256];
	int len = 0;
	unsigned int crc;
	int rc = -1;

	if ( ( address & ModBus::VT_MASK ) == ModBus::VT_OUTPUT_COIL ) {
		assert(0);
	} else if ( ( address & ModBus::VT_MASK ) == ModBus::VT_INPUT_CONTACT ) {
		assert(0);
	} else if ( ( address & ModBus::VT_MASK ) == ModBus::VT_INPUT_REGISTER ) {

		address &= ~ModBus::VT_MASK;

		buffer[len++] = m_id;
		buffer[len++] = 0x10;
		buffer[len++] = ( (address) >> 8 ) & 0xFF;
		buffer[len++] = ( (address)      ) & 0xFF;
		buffer[len++] = ( (count) >> 8 ) & 0xFF;
		buffer[len++] = ( (count) ) & 0xFF;
		buffer[len++] = count * 2;
		for ( unsigned int i = 0; i < count; i++ ) {
			buffer[len++] = data[i].rawHI();
			buffer[len++] = data[i].rawLO();
		}
		crc = calc_crc( buffer, len );
		buffer[len++] = ( crc >> 8 ) & 0xFF;
		buffer[len++] = ( crc ) & 0xFF;
		len = transact( buffer, len, 8 );
		if ( len >= 0 ) {
			rc = 0;
		}
	} else if ( ( address & ModBus::VT_MASK ) == ModBus::VT_OUTPUT_REGISTER ) {
		assert(0);
	} else {
		rc = -1;
	}
	return rc;
}

ModBus::ModBus( const char* ip, unsigned short port, unsigned int id ) {
	m_ip = strdup( ip );
	m_port = port;
	m_id = id;
}

ModBus::~ModBus() {
	free( (void*)m_ip );
}


