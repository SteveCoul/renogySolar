
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
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

#include "Args.hpp"
#include "Common.hpp"
#include "ModBus.hpp"
#include "renogy.hpp"

///
/// \class Controller
///
/// \brief  A Task to provide a controllers status as an XML file on incoming HTTP request.
///
/// It runs a TCP server and on connection will read a solar charge controllers status
/// from a given port and return the XML with a suitable HTTP protocol
/// response.
///
/// \see RS485TCP
///
class Controller {
public:

    static
    int main( Args* args ) {
    
        // FIXME proper arg parsing
        int m_server_fd;
    
        int rport = args->getOptionAsInt( "rp" );
        const char* raddr = args->getOptionAsString( "ra" );
        int id = args->getOptionAsInt( "i" );
        int myport = args->getOptionAsInt( "p" );
    
        signal( SIGPIPE, SIG_IGN );

        log( LOG_INFO, "Use rs485 server at %s:%d, and query renogy device %d, serving on port %d", raddr, rport, myport );

        ModBus modbus( raddr, rport, id );
    
        m_server_fd = Common::createTCPServerSocket( myport );
        if ( m_server_fd >= 0 ) {
            while(1) {      // No way or need to quit atm
                int client_fd = Common::tcpAccept( m_server_fd );
                if ( client_fd > 0 ) {
                    ModBus::Value value[3];

                    // TODO handle error/timeout on modbus read
    
                    time_t  now;
                    struct tm* tm;
    
                    (void)time(&now);
                    tm = localtime( &now );
    
                    unsigned int    local_year = tm->tm_year + 1900;
                    unsigned int    local_month = tm->tm_mon + 1;
                    unsigned int    local_day = tm->tm_mday;
                    unsigned int    local_hour = tm->tm_hour;
                    unsigned int    local_minute = tm->tm_min;
                    unsigned int    local_seconds = tm->tm_sec;
                    int local_time = ( local_hour * 3600 ) + ( local_minute * 60 ) + local_seconds;
    
                    modbus.readVariable( RENOGY_RTC_MINUTE_SECOND, 1, 3, value ); 
    
                    unsigned int    remote_year = value[2].rawHI() + 2000;
                    unsigned int    remote_month = value[2].rawLO();
                    unsigned int    remote_day = value[1].rawHI();
                    unsigned int    remote_hour = value[1].rawLO();
                    unsigned int    remote_minute = value[0].rawHI();
                    unsigned int    remote_seconds = value[0].rawLO();
                    int remote_time = ( remote_hour * 3600 ) + ( remote_minute * 60 ) + remote_seconds;
    
                    int time_difference = abs( local_time - remote_time );
    
                    if ( ( local_year != remote_year ) ||
                         ( local_month != remote_month ) ||
                         ( local_day != remote_day ) ||
                         ( time_difference > 30 ) ) {
    
                        value[2].set( local_year - 2000, local_month );
                        value[1].set( local_day, local_hour );
                        value[0].set( local_minute, local_seconds );
                    
                        (void)modbus.writeRawVariable( RENOGY_RTC_MINUTE_SECOND, 3, value );
                    }
    
                    unsigned int tmp;
                    char* body = NULL;
                    char* head = NULL;
    
                    Common::mprintf( &body, "<controller id=\"%d\">\n", id );
    
                    Common::mprintf( &body, "\t<pv_array_rating>\n");
                    (void)modbus.readVariable( RENOGY_RATED_INPUT_VOLTAGE, 100, 1, value ); 
                    Common::mprintf( &body, "\t\t<voltage>%f</voltage>\n", value[0].asFloat() );
                
                    (void)modbus.readVariable( RENOGY_RATED_INPUT_CURRENT, 100, 1, value ); 
                    Common::mprintf( &body, "\t\t<current>%f</current>\n", value[0].asFloat() );
                    
                    (void)modbus.readVariable( RENOGY_RATED_INPUT_POWER, 100, 2, value ); 
                    tmp = ( value[1].raw() << 16 ) | value[0].raw();
                    Common::mprintf( &body, "\t\t<power>%f</power>\n", ((float)tmp)/100.0f );
                    Common::mprintf( &body, "\t</pv_array_rating>\n");
                    
                    Common::mprintf( &body, "\t<pv_array_now>\n");
                    (void)modbus.readVariable( RENOGY_PV_INPUT_VOLTAGE, 100, 1, value ); 
                    Common::mprintf( &body, "\t\t<voltage>%f</voltage>\n", value[0].asFloat() );
                    
                    (void)modbus.readVariable( RENOGY_PV_INPUT_CURRENT, 100, 1, value ); 
                    Common::mprintf( &body, "\t\t<current>%f</current>\n", value[0].asFloat() );
    
                    (void)modbus.readVariable( RENOGY_PV_INPUT_POWER, 100, 2, value ); 
                    tmp = ( value[1].raw() << 16 ) | value[0].raw();
                    Common::mprintf( &body, "\t\t<power>%f</power>\n", ((float)tmp)/100.0f );
                    Common::mprintf( &body, "\t</pv_array_now>\n");
                    
                    Common::mprintf( &body, "\t<battery>\n");
                    (void)modbus.readVariable( RENOGY_BATTERY_VOLTAGE, 100, 1, value ); 
                    Common::mprintf( &body, "\t\t<voltage>%f</voltage>\n", value[0].asFloat() );
    
                    (void)modbus.readVariable( RENOGY_BATTERY_CHARGING_CURRENT, 100, 1, value ); 
                    Common::mprintf( &body, "\t\t<current>%f</current>\n", value[0].asFloat() );
    
                    (void)modbus.readVariable( RENOGY_BATTERY_STATE_OF_CHARGE, 1, 1, value ); 
                    Common::mprintf( &body, "\t\t<state_of_charge>%f</state_of_charge>\n", value[0].asFloat() );
    
                    (void)modbus.readVariable( RENOGY_NET_BATTERY_CURRENT_L, 100, 2, value ); 
                    tmp = ( value[1].raw() << 16 ) | value[0].raw();
                    Common::mprintf( &body, "\t\t<net_current>%f</net_current>\n", ((float)tmp)/100.0f );
                    Common::mprintf( &body, "\t</battery>\n");
    
                    Common::mprintf( &body, "\t<generation>\n");
                    (void)modbus.readVariable( RENOGY_GENERATED_ENERGY_TODAY_L, 100, 2, value ); 
                    tmp = ( value[1].raw() << 16 ) | value[0].raw();
                    Common::mprintf( &body, "\t\t<today>%f</today>\n", ((float)tmp)/100.0f );
    
                    (void)modbus.readVariable( RENOGY_GENERATED_ENERGY_MONTH_L, 100, 2, value ); 
                    tmp = ( value[1].raw() << 16 ) | value[0].raw();
                    Common::mprintf( &body, "\t\t<this_month>%f</this_month>\n", ((float)tmp)/100.0f );
    
                    (void)modbus.readVariable( RENOGY_GENERATED_ENERGY_YEAR_L, 100, 2, value ); 
                    tmp = ( value[1].raw() << 16 ) | value[0].raw();
                    Common::mprintf( &body, "\t\t<this_year>%f</this_year>\n", ((float)tmp)/100.0f );
                    Common::mprintf( &body, "\t</generation>\n");
    
                    Common::mprintf( &body, "</controller>\n");
    
                    Common::mprintf( &head, "HTTP/1.0 200 OK\r\n");
                    Common::mprintf( &head, "Access-Control-Allow-Origin: *\r\n");
                    Common::mprintf( &head, "Content-Length: %d\r\n", strlen( body ) );
                    Common::mprintf( &head, "\r\n");
    
                    write( client_fd, head, strlen(head) );
                    write( client_fd, body, strlen(body) );

                    free( (void*)body );
                    free( (void*)head );
    
                    Common::waitForTCPHangup( client_fd );
                    close( client_fd );
                }
            }
        }
        return 0;
    }

};

ENTRYPOINT( Controller )
DEFAULT_ARGS(   "-ra:IP address of remote serial port controller:127.0.0.1"
                "-rp:TCP port for serial controller:32700"
                "-i:ID of charge controller to operate on:1"
                "-p:TCP port to listen on:32701" )


