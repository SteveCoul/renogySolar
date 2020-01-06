#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/poll.h>

#include <iomanip>
#include <sstream>

#include "Common.hpp"
#include "Log.hpp"
#include "SerialPort.hpp"

SerialPort::SerialPort( const char* device, unsigned int baud_rate )    :   m_baud(B115200)
                                                                        ,   m_device(NULL)
                                                                        ,   m_fd(-1) {
    m_device = strdup( device );
    if ( m_device == NULL ) {
        log( LOG_ERR, "out of memory creating SerialPort" );
    }

    switch( baud_rate ) {
    case 115200:
        m_baud = B115200;
        break;
    default:
        m_baud = B115200;
        log( LOG_WARNING, "Unknown baud rate, using 115200" );
        break;
    }
}

SerialPort::~SerialPort() {
    (void)close();
    if ( m_device ) free( (void*)m_device );
}

int SerialPort::open() {
    int rc = -1;
    struct termios  term;
    m_fd = ::open( m_device, O_RDWR | O_NDELAY | O_NOCTTY );
    if ( m_fd < 0 ) {
        log( LOG_CRIT, "Failed to open %s [%s]\n", m_device, strerror(errno) );
    } else {
        if ( tcgetattr( m_fd, &term ) < 0 ) {
            log( LOG_CRIT, "Failed to get serial config : %s\n", strerror(errno) );
        } else {
            int opt_dtr = TIOCM_DTR;
            int opt_rts = TIOCM_RTS;
    
            cfmakeraw( &term );
            cfsetispeed( &term, m_baud );
            cfsetospeed( &term, m_baud );
            if ( tcsetattr( m_fd, TCSANOW, &term ) < 0 ) {
                log( LOG_CRIT, "Failed to set serial config ; %s\n", strerror(errno) );
            } else if ( ::ioctl( m_fd, TIOCMBIS, &opt_dtr ) < 0 ) {
                log( LOG_CRIT, "Failed to set DTR : %s\n", strerror(errno) );
            } else if ( ::ioctl( m_fd, TIOCMBIS, &opt_rts ) < 0 ) {
                log( LOG_CRIT, "Failed to set RTS : %s\n", strerror(errno) );
            } else if ( ::tcflush( m_fd, TCIFLUSH ) < 0 ) {
                log( LOG_CRIT, "Failed iflush : %s\n", strerror(errno) );
            } else {
                rc = 0;
            }
        }
    }
    return rc;
}

int SerialPort::close() {
    if ( m_fd >= 0 ) {
        ::close( m_fd );
        m_fd = -1;
    }
    return 0;
}

int SerialPort::transact( int fd, int time_out ) {
    int rc = 0;

    if ( m_fd < 0 ) {
        log( LOG_ERR, "failing serial transaction, port not open" );
        return -1;
    }

    unsigned long long start_time = Common::NOWms();

    for (;;) {
        unsigned long long now = Common::NOWms();
        int how_long;

        if ( time_out >= 0 ) { 
            if ( ( now-start_time) > (unsigned)time_out ) {
                log( LOG_WARNING, "serial transaction timed out after %d ms", time_out );
                errno = ETIMEDOUT;
                return -1;
            }

            how_long = time_out - ( now-start_time );
        } else {
            how_long = 300;
        }

        struct pollfd pfd[2];
        pfd[0].fd = fd;
        pfd[0].events = POLLIN | POLLHUP | POLLERR;
        pfd[0].revents = 0;
        pfd[1].fd = m_fd;
        pfd[1].events = POLLIN | POLLHUP | POLLERR;
        pfd[1].revents = 0;

        if ( poll( pfd, 2, how_long ) < 0 ) {
            log( LOG_WARNING, "Poll error during serial transaction [%s]", strerror(errno) );
            rc = -1;
            break;
        } else {

            if ( Common::shouldQuit() ) {
                errno = EINTR;
                rc = -1;
                break;
            }
            unsigned char buffer[1024];
            int len;
    
            if ( pfd[0].revents & POLLIN ) {
                len = read( pfd[0].fd, buffer, sizeof(buffer) );
                if ( len == 0 ) {
                    log( LOG_DEBUG, "serialport transaction client closed (no more data)" );
                    break;
                }
                if ( len < 0 ) {
                    if ( errno == ECONNRESET ) {
                        // client closed/reset
                        log( LOG_DEBUG, "serialport tranaction client closed (reset)" );
                        break;  
                    }
                    log( LOG_WARNING, "serialport transaction client read error [%s]", strerror(errno) );
                    rc = -1;
                    break;
                }

    			std::stringstream   dbg;
				dbg << m_device << " < ";
                for ( int i = 0; i < len; i++ ) dbg << std::hex << std::setw(2) << std::setfill('0') << static_cast<unsigned int>(buffer[i]) << " ";
                log( LOG_DEBUG, dbg.str() );
    
                if ( write( m_fd, buffer, len ) != len ) {
                    log( LOG_WARNING, "serialport write failed [%s]", strerror(errno) );
                    rc = -1;
                    break;
                }
            }

            if ( pfd[0].revents & POLLERR ) {
                log( LOG_WARNING, "serialport client error [%s]", strerror(errno) );
                rc = -1;
                break;
            }

            if ( pfd[0].revents & POLLHUP ) {
                log( LOG_DEBUG, "serialport transaction client hangup" );
                break;
            }

            if ( pfd[1].revents & POLLIN ) {
                len = read( m_fd, buffer, sizeof(buffer) );
                if ( len < 0 ) {
                    log( LOG_ERR, "serialport read error [%s]", strerror(errno) );
                    rc = -1;
                    break;
                }
    
    			std::stringstream   dbg;
				dbg << m_device << " > ";
                for ( int i = 0; i < len; i++ ) dbg << std::hex << std::setw(2) << std::setfill('0') << static_cast<unsigned int>(buffer[i]) << " ";
                log( LOG_DEBUG, dbg.str() );

                if ( write( fd, buffer, len ) != len ) {
                    log( LOG_WARNING, "serialport transaction client write problem [%s]", strerror(errno) );
                    rc = -1;
                    break;
                }
            }

            if ( pfd[1].revents & POLLERR ) {
                log( LOG_WARNING,"serial error [%s]", strerror(errno) );
                rc = -1;
                break;
            }
            if ( pfd[1].revents & POLLHUP ) {
                log( LOG_WARNING, "serial hungup [%s]", strerror(errno) );
                rc = -1;
                break;
            }
        }
    }
    return rc;
}

