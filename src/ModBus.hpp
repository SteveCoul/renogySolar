#ifndef __modbus_hpp__
#define __modbus_hpp__

class ModBus {
public:
    class Value {
    private:
        unsigned int    m_raw;
        unsigned int    m_rawHI;
        unsigned int    m_rawLO;
        unsigned int    m_scale;
        bool            m_asFlag;
        float           m_asFloat;
        unsigned int    m_asInteger;
    public:
        Value() { invalidate(); }
        ~Value() { }
        void set( unsigned char hi, unsigned char lo, unsigned int scale = 1 );
        void invalidate( void );
        unsigned int rawHI() { return m_rawHI; }
        unsigned int rawLO() { return m_rawLO; }
        float asFloat() { return m_asFloat; }
        unsigned int raw() { return m_raw; }
    };
public:
    static const int VT_MASK            =   0xFF000000;
    static const int VT_OUTPUT_COIL     =   0x01000000;
    static const int VT_INPUT_CONTACT   =   0x02000000;
    static const int VT_INPUT_REGISTER  =   0x03000000;
    static const int VT_OUTPUT_REGISTER =   0x04000000;
    static const int VT_UNKNOWN         =   0xFF000000;
public:
    ModBus( const char* ip, unsigned short port, unsigned int id );
    ~ModBus();

    int readVariable( unsigned int address, unsigned int scale, unsigned int count, Value* p_result );
    int writeRawVariable( unsigned int address, unsigned int count, Value* data );
    void setValues( int scale, int count, Value* p, unsigned char* stream );
    void setInvalidValues( int scale, int count, Value* p );
private:    
    int transact( unsigned char* buffer, int send_len, int rx_len );
    static unsigned int calc_crc( unsigned char* buffer, int len );
    static int writeComplete( int fd, unsigned char* buffer, int len );
    static int readComplete( int fd, unsigned char* buffer, int len );
private:
    const char*     m_ip;
    unsigned short  m_port;
    unsigned int    m_id;
};
        

#endif

