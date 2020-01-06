
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
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

#include <iomanip>
#include <iostream>
#include <sstream>

#include "Args.hpp"
#include "Common.hpp"
#include "Log.hpp"
#include "ModBus.hpp"
#include "renogy.hpp"
#include <HTTPServer.hpp>

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
class Controller : public HTTPServerImplementation {
public:

    static
    int main( Args* args ) {
   
        Controller c(args);
        return c.run();
    }

    Controller( Args* args ) {
        int rport = args->getOptionAsInt( "rp" );
        const char* raddr = args->getOptionAsString( "ra" );
        m_id = args->getOptionAsInt( "i" );
        m_modbus = new ModBus( raddr, rport, m_id );
        int myport = args->getOptionAsInt( "p" );
        m_httpserver = new HTTPServer( myport );
    }

    ~Controller() {
        delete m_modbus;
        delete m_httpserver;
    }

    int run() {
        fixTime();
        m_httpserver->setHandler( this );
        while (Common::shouldQuit() == 0 ) {        
            m_httpserver->process();
        }
        return 0;
    }
         
    int HTTPServerRequest( std::string path, std::string query, std::string& response, std::string& content_type, std::string& body ) {
        if ( path.compare("/data.xml" ) != 0 ) {
            response = "Not Found";
            return 404;
        }

        m_body.str("");
        m_body << "<controller id=\"" << m_id << "\">\n"; 

        m_body << "\t<pv_array_rating>\n"; 
        singleFloatVariable( RENOGY_RATED_INPUT_VOLTAGE, 100, "\t\t<voltage>", "</voltage>\n" );
        singleFloatVariable( RENOGY_RATED_INPUT_CURRENT, 100, "\t\t<current>", "</current>\n" );
        doubleFloatVariable( RENOGY_RATED_INPUT_POWER, 100, "\t\t<power>", "</power>\n" );
        m_body << "\t</pv_array_rating>\n"; 

        m_body << "\t<pv_array_now>\n"; 
        singleFloatVariable( RENOGY_PV_INPUT_VOLTAGE, 100, "\t\t<voltage>", "</voltage>\n" );
        singleFloatVariable( RENOGY_PV_INPUT_CURRENT, 100, "\t\t<current>", "</current>\n" );
        doubleFloatVariable( RENOGY_PV_INPUT_POWER, 100, "\t\t<power>", "</power>\n" );
        m_body << "\t</pv_array_now>\n"; 

        m_body << "\t<battery>\n"; 
        singleFloatVariable( RENOGY_BATTERY_VOLTAGE, 100, "\t\t<voltage>", "</voltage>\n" );
        singleFloatVariable( RENOGY_BATTERY_CHARGING_CURRENT, 100, "\t\t<current>", "</current>\n" );
        singleFloatVariable( RENOGY_BATTERY_STATE_OF_CHARGE, 1, "\t\t<state_of_charge>", "</state_of_charge>\n" );
        doubleFloatVariable( RENOGY_NET_BATTERY_CURRENT_L, 100, "\t\t<net_current>", "</net_current>\n" );
        m_body << "\t</battery>\n"; 

        m_body << "\t<generation>\n"; 
        doubleFloatVariable( RENOGY_GENERATED_ENERGY_TODAY_L, 100, "\t\t<today>", "</today>\n" );
        doubleFloatVariable( RENOGY_GENERATED_ENERGY_MONTH_L, 100, "\t\t<this_month>", "</this_month>\n" );
        doubleFloatVariable( RENOGY_GENERATED_ENERGY_YEAR_L, 100, "\t\t<this_year>", "</this_year>\n" );
        m_body << "\t</generation>\n"; 
                    
        m_body << "</controller>\n"; 
 
        content_type = "text/xml";
        body = m_body.str();
        response = "OK";
        return 200;
    }

private:

    void singleFloatVariable( int reg, int scale, const char* prefix, const char* postfix ) {
        ModBus::Value   value;
        (void)m_modbus->readVariable( reg, scale, 1, &value );
        m_body << prefix << value.asFloat() << postfix;
    }

    void doubleFloatVariable( int reg, int scale, const char* prefix, const char* postfix ) {
        ModBus::Value   value[2];
        (void)m_modbus->readVariable( reg, scale, 2, value );

        unsigned int tmp;
        tmp = ( value[1].raw() << 16 ) | value[0].raw();

        float tmpf = ((float)tmp)/scale;
        m_body << prefix << tmpf << postfix;
    }

    void fixTime() {
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

        m_modbus->readVariable( RENOGY_RTC_MINUTE_SECOND, 1, 3, value ); 

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
        
            (void)m_modbus->writeRawVariable( RENOGY_RTC_MINUTE_SECOND, 3, value );
        }
    }
  
private:
    std::stringstream   m_body;
    int                 m_id;
    ModBus*             m_modbus;
    HTTPServer*         m_httpserver;
};
             
ENTRYPOINT( Controller )
DEFAULT_ARGS(   "-ra:IP address of remote serial port controller:127.0.0.1"
                "-rp:TCP port for serial controller:32700"
                "-i:ID of charge controller to operate on:1"
                "-p:TCP port to listen on:32701" )


