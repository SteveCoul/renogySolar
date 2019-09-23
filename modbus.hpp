#ifndef __modbus_hpp__
#define __modbus_hpp__

typedef struct {
	unsigned int	raw;
	unsigned int	rawHI;
	unsigned int	rawLO;
	unsigned int	scale;
	bool			asFlag;
	float			asFloat;
	unsigned int	asInteger;
} modbus_data_value_t;

#define MODBUS_VT_MASK 0xFF000000
enum {
	MODBUS_VT_OUTPUT_COIL		=	0x01000000,
	MODBUS_VT_INPUT_CONTACT		=	0x02000000,
	MODBUS_VT_INPUT_REGISTER	=	0x03000000,
	MODBUS_VT_OUTPUT_REGISTER	=	0x04000000,
	MODBUS_VT_UNKNOWN			=	0xFF000000
};

int modbusReadVariable( const char* ip, unsigned short port, unsigned int unit, unsigned int address, unsigned int scale, unsigned int count, modbus_data_value_t* p_result );

int modbusWriteRawVariable( const char* ip, unsigned short port, unsigned int unit, unsigned int address, unsigned int count, modbus_data_value_t* data );

#endif

