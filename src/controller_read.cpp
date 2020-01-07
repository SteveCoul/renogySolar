
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

#include <Args.hpp>
#include <HTTPServer.hpp>
#include <Log.hpp>
#include <ModBus.hpp>
#include <Renogy.hpp>
#include <Task.hpp>
#include <XML2JSON.hpp>

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

    /// \brief  Entrypoint, Create a controller class and run it.
    /// \return exit code.
    static
    int main( Args* args ) {
        Controller c(args);
        return c.run();
    }

    /// \brief  Construct controller task.
    /// \param  args    Command line arguments.
    Controller( Args* args ) {
        int rport = args->getOptionAsInt( "rp" );
        const char* raddr = args->getOptionAsString( "ra" );
        m_id = args->getOptionAsInt( "i" );
        m_modbus = new ModBus( raddr, rport, m_id );
        int myport = args->getOptionAsInt( "p" );
        m_httpserver = new HTTPServer( myport );
    }

    /// \brief  Destroy object.
    ~Controller() {
        delete m_modbus;
        delete m_httpserver;
    }

    /// \brief  Run the task until quit.
    /// First ensures remote device has the correct time of day, then runs a
    /// webserver which will respond to requests for data.
    /// \return -ve on error.
    int run() {
        fixTime();
        m_httpserver->setHandler( this );
        while (Task::shouldQuit() == 0 ) {        
            m_httpserver->process();
        }
        return 0;
    }
        
    /// \brief  Action an incoming HTTP request.
    /// As is, will serve two files /data.xml and /data.json which represent the current state
    /// of a charge controller using ModBus to query the hardware.
    /// \param  path            File requested.
    /// \param  query           Any query string on URL.
    /// \param  response        Where to store http response line text.
    /// \param  content_type    Where to store resulting content type field.
    /// \param  body            Where to store any body for response.
    /// \return http response code. 
    int HTTPServerRequest( std::string path, std::string query, std::string& response, std::string& content_type, std::string& body ) {
        if ( ( path.compare("/data.xml" ) != 0 ) && ( path.compare("/data.json" ) != 0 ) ) {
            response = "Not Found";
            return 404;
        }

        m_body.str("");
        m_body << "<controller id=\"" << m_id << "\">\n"; 

        m_body << "\t<pv_array_rating>\n"; 
        singleFloatVariable( Renogy::RATED_INPUT_VOLTAGE, 100, "\t\t<voltage>", "</voltage>\n" );
        singleFloatVariable( Renogy::RATED_INPUT_CURRENT, 100, "\t\t<current>", "</current>\n" );
        doubleFloatVariable( Renogy::RATED_INPUT_POWER, 100, "\t\t<power>", "</power>\n" );
        m_body << "\t</pv_array_rating>\n"; 

        m_body << "\t<pv_array_now>\n"; 
        singleFloatVariable( Renogy::PV_INPUT_VOLTAGE, 100, "\t\t<voltage>", "</voltage>\n" );
        singleFloatVariable( Renogy::PV_INPUT_CURRENT, 100, "\t\t<current>", "</current>\n" );
        doubleFloatVariable( Renogy::PV_INPUT_POWER, 100, "\t\t<power>", "</power>\n" );
        m_body << "\t</pv_array_now>\n"; 

        m_body << "\t<battery>\n"; 
        singleFloatVariable( Renogy::BATTERY_VOLTAGE, 100, "\t\t<voltage>", "</voltage>\n" );
        singleFloatVariable( Renogy::BATTERY_CHARGING_CURRENT, 100, "\t\t<current>", "</current>\n" );
        singleFloatVariable( Renogy::BATTERY_STATE_OF_CHARGE, 1, "\t\t<state_of_charge>", "</state_of_charge>\n" );
        doubleFloatVariable( Renogy::NET_BATTERY_CURRENT_L, 100, "\t\t<net_current>", "</net_current>\n" );
        m_body << "\t</battery>\n"; 

        m_body << "\t<generation>\n"; 
        doubleFloatVariable( Renogy::GENERATED_ENERGY_TODAY_L, 100, "\t\t<today>", "</today>\n" );
        doubleFloatVariable( Renogy::GENERATED_ENERGY_MONTH_L, 100, "\t\t<this_month>", "</this_month>\n" );
        doubleFloatVariable( Renogy::GENERATED_ENERGY_YEAR_L, 100, "\t\t<this_year>", "</this_year>\n" );
        m_body << "\t</generation>\n"; 
                    
        m_body << "</controller>\n"; 

        if ( path.compare("/data.xml") == 0 ) { 
            content_type = "text/xml";
            body = m_body.str();
        } else {
            content_type = "text/json";
            XML2JSON json;
            std::string body_string = m_body.str();
            body = json.convert( body_string );
        }
        response = "OK";
        return 200;
    }

private:

    /// \brief  Read a single register over Modbus, and output suitable text for the XML output via m_body.
    /// \param  reg     Modbus Register to read.
    /// \param  scale   Scale to apply.
    /// \param  prefix  Start text to write.
    /// \param  postfix End text to write after value.
    void singleFloatVariable( int reg, int scale, const char* prefix, const char* postfix ) {
        ModBus::Value   value;
        (void)m_modbus->readVariable( reg, scale, 1, &value );
        m_body << prefix << value.asFloat() << postfix;
    }

    /// \brief  Read a pair of registers over Modbus, and output suitable text for the XML output via m_body.
    /// \param  reg     First Modbus Register to read.
    /// \param  scale   Scale to apply.
    /// \param  prefix  Start text to write.
    /// \param  postfix End text to write after value.
    void doubleFloatVariable( int reg, int scale, const char* prefix, const char* postfix ) {
        ModBus::Value   value[2];
        (void)m_modbus->readVariable( reg, scale, 2, value );

        unsigned int tmp;
        tmp = ( value[1].raw() << 16 ) | value[0].raw();

        float tmpf = ((float)tmp)/scale;
        m_body << prefix << tmpf << postfix;
    }

    /// \brief  Query device over modbus for time-of-day and update it with local time if required.
    void fixTime() {
        ModBus::Value value[3];

        /// \todo handle error/timeout on modbus read

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

        m_modbus->readVariable( Renogy::RTC_MINUTE_SECOND, 1, 3, value ); 

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
        
            (void)m_modbus->writeRawVariable( Renogy::RTC_MINUTE_SECOND, 3, value );
        }
    }
  
private:
    std::stringstream   m_body;         ///< A stream where the XML output is generated on request.
    int                 m_id;           ///< Modbus ID for device this controller operates on.
    ModBus*             m_modbus;       ///< Modbus class.
    HTTPServer*         m_httpserver;   ///< HTTP server.
};
             
ENTRYPOINT( Controller )
DEFAULT_ARGS(   "-ra:IP address of remote serial port controller:127.0.0.1"
                "-rp:TCP port for serial controller:32700"
                "-i:ID of charge controller to operate on:1"
                "-p:TCP port to listen on:32701" )


