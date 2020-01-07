#ifndef __modbus_hpp__
#define __modbus_hpp__

///
/// \class ModBus
///
/// \brief  Implements the ModBus protocol over a TCP link to the rs485 controller task.
///
class ModBus {
public:
    ///
    /// \class Value
    ///
    /// \brief  Represents a register value.
    ///
    class Value {
    private:
        unsigned int    m_raw;          ///<    Raw 16bit encoded value 
        unsigned int    m_rawHI;        ///<    Upper 8 bits
        unsigned int    m_rawLO;        ///<    Lower 8 bits
        unsigned int    m_scale;        ///<    Scale factor to apply to value
        bool            m_asFlag;       ///<    Value decoded as a boolean
        float           m_asFloat;      ///<    Value decoded as a float
        unsigned int    m_asInteger;    ///<    Value as integer
    public:
        /// \brief  Construct value and initialize with all 0/off/false.
        Value() { invalidate(); }

        /// \brief  Destructor
        ~Value() { }

        /// \brief  Set a value using HI/LO 8 bit values and a scale to apply.
        ///
        /// \param  hi      Upper 8 bits of raw integer
        /// \param  lo      Lower 8 bits of raw integer
        /// \param  scale   Divisor for decoded value.
        void set( unsigned char hi, unsigned char lo, unsigned int scale = 1 );

        /// \brief  Return a value to the default construction state.
        void invalidate( void );

        /// \brief  Return upper 8 bits of encoded value.
        /// \return value
        unsigned int rawHI() { return m_rawHI; }

        /// \brief  Return lower 8 bits of encoded value.
        /// \return value
        unsigned int rawLO() { return m_rawLO; }

        /// \brief  Return register value when decoded as a float.
        /// \return value
        float asFloat() { return m_asFloat; }

        /// \brief  Return raw register value.
        /// \return value
        unsigned int raw() { return m_raw; }
    };
public:
    static const int VT_MASK            =   0xFF000000;     ///< Mask for register type
    static const int VT_OUTPUT_COIL     =   0x01000000;     ///< Modbus Register Type
    static const int VT_INPUT_CONTACT   =   0x02000000;     ///< Modbus Register Type
    static const int VT_INPUT_REGISTER  =   0x03000000;     ///< Modbus Register Type
    static const int VT_OUTPUT_REGISTER =   0x04000000;     ///< Modbus Register Type
    static const int VT_UNKNOWN         =   0xFF000000;     ///< Invalid Modbus Register Type
public:
    /// \brief  Create instance and connect it to the given rs485 task for a given device address.
    /// \param  ip      IP address of RS485 task.
    /// \param  port    IP port of RS485 task.
    /// \param  id      Modbus device address for target.
    ModBus( std::string ip, unsigned short port, unsigned int id );

    /// \brief  Destructor
    ~ModBus();

    /// \brief  Read a number of consecutive Modbus variables.
    /// \param  address     First register ( offset | type ) to read.
    /// \param  scale       Scaling to apply to resulting values.
    /// \param  count       How many registers to read.
    /// \param  p_result    Array of value objects to store results.
    /// \return -ve on error.
    int readVariable( unsigned int address, unsigned int scale, unsigned int count, Value* p_result );

    /// \brief  Write registers on device.
    /// \param  address     First register ( offset | type ) to write.
    /// \param  count       Number of registers.
    /// \param  data        Values to write. 
    /// \return -ve on error.
    int writeRawVariable( unsigned int address, unsigned int count, Value* data );

private:    
    /// \brief  Set a number of values using a modbus encoded byte stream.
    /// \param  scale       Scale to apply to values.
    /// \param  count       How many values to decode from stream.
    /// \param  p           Where to store values.
    /// \param  stream      Start of byte stream.
    void setValues( int scale, int count, Value* p, unsigned char* stream );

    /// \brief  Invalidate an array of value objects.
    /// \param  scale       Scale to apply. (TODO why?)
    /// \param  count       Number of objects.
    /// \param  p           Array of values.
    void setInvalidValues( int scale, int count, Value* p );

    /// \brief  Perform a modbus transaction. Send a packet, wait for response.
    /// \param  buffer      Buffer. On call contains send_len bytes to data to transmit. On success contains rx_len of data.
    /// \param  send_len    Number of bytes to send to device.
    /// \param  rx_len      Expected number of bytes to return.
    /// \return -ve on error ( including timeout ).
    int transact( unsigned char* buffer, int send_len, int rx_len );

private:
    /// \brief  Calculate a modbus CRC on a buffer.
    /// \param  buffer      Data to CRC.
    /// \param  len         Number of bytes to CRC.
    /// \return 16bit CRC value for buffer.
    static unsigned int calc_crc( unsigned char* buffer, int len );

    /// \brief  Write a number of bytes to a file-descriptor.
    /// \param  fd          Where to write data.
    /// \param  buffer      Data to write.
    /// \param  len         Amount to write.
    /// \return -ve on error.
    static int writeComplete( int fd, unsigned char* buffer, int len );

    /// \brief  Read an exact number of bytes from a file-descriptor.
    /// \param  fd          Where to read from.
    /// \param  buffer      Where to store data.
    /// \param  len         Amount to read.
    /// \return -ve on error.
    static int readComplete( int fd, unsigned char* buffer, int len );

    /// \brief  Perform a read with an optional timeout value.
    /// \param  fd          Where to read from.
    /// \param  buffer      Where to store data.
    /// \param  length      Amount to read.
    /// \param  timeout_ms  Maximum number of milliseconds to wait for read. -ve == forever.
    /// \return -ve on error including timeout.
    static int timedRead( int fd, void* buffer, size_t length, int timeout_ms );
private:
    std::string     m_ip;           ///< IP address string (dotted numeric) for the RS485 task used.
    unsigned short  m_port;         ///< Port number for the RS485 task used.
    unsigned int    m_id;           ///< Modbus ID for the device operating on.
};
        

#endif

