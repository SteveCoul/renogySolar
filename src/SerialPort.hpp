#ifndef __SerialPort_hpp__
#define __SerialPort_hpp__

#include <termios.h>

#include <string>

///
/// \class SerialPort
///
/// \brief Class to control serial device and manage transmission and reception of packet data.
///
class SerialPort {
public:
    ///
    /// \brief  Create instance for a given device and configuration.
    ///
    /// \param  device      Path to serial port device
    /// \param  baud_rate   Serial port speed
    ///
    SerialPort( std::string device, unsigned int baud_rate );

    ///
    /// \brief  destructor
    ///
    ~SerialPort();

    ///
    /// \brief  Open serial device.
    ///
    /// \return     -ve on error
    ///
    int open();

    ///
    /// \brief  Close serial device.
    ///
    /// \return     -ve on error
    ///
    int close();

    ///
    /// \brief  Perform a transaction with a client on a given file stream.
    ///
    /// Will read data from the client and send it to the serial port whilst returning any incoming serial data
    /// to the client file descriptor. This will continue until either the time_out value is passed ( which will cause
    /// an ETIMEDOUT error ) or until the client descriptor closes/hangsup.
    ///
    /// \param  fd          client file descriptor
    /// \param  time_out    maximum number of milliseconds to perform transaction before failing. -ve for infinite.
    ///
    /// \return     -ve on error
    ///
    int transact( int fd, int time_out );

private:    
    /// Configured baud rate
    speed_t     m_baud;
    /// Device path
    std::string m_device;
    /// Device file descriptor
    int         m_fd;
};

#endif

