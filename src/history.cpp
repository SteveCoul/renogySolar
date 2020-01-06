
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <stdlib.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <time.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <sys/poll.h>

#include <sqlite3.h>

#include <thread>

#include "Task.hpp"
#include "HistoryTable.hpp"
#include "HTTPServer.hpp"
#include "Log.hpp"
#include "ModBus.hpp"
#include <TCP.hpp>
#include "XML2JSON.hpp"
#include "Renogy.hpp"

class History  : public HTTPServerImplementation {
public:

    int HTTPServerRequest( std::string path, std::string query, std::string& response, std::string& content_type, std::string& body ) {

        size_t dot = path.rfind( '.' );
        size_t slash = path.rfind( '/' );
        size_t second_slash = path.rfind( '/', slash-1 );

        std::string ext = path.substr( dot+1 );
        std::string base = path.substr( slash+1, dot-slash-1 );
        int id = std::stoi( path.substr( second_slash+1, slash-second_slash-1 ) );

        HistoryTable* tab = HistoryTable::get( base.c_str() );
        if ( tab != NULL ) {
            std::string xml = tab->toXML( id );
            if ( ext.compare( "xml" ) == 0 ) {
                body = xml;
                response = "Ok";
                return 200;
            }

            if ( ext.compare( "json" ) == 0 ) {
                XML2JSON json;
                body = json.convert( xml );
                response = "Ok";
                return 200;
            }
        }
        response = "Not Found";
        return 404;
    }

    void httpThread( int port ) {
        HTTPServer  server( port );
        server.setHandler( this );

        while ( Task::shouldQuit() == 0 ) {
            server.process();
        }
    }

    static
    int main( Args* args ) {
        HistoryTable::cinit( args->getOptionAsString("f") );
        History h;
        return h.run( args );
    }

    int run( Args* args ) {
        std::thread http( &History::httpThread, this, args->getOptionAsInt("p") );

        HistoryTable* t_recent = new HistoryTable( "Recent" );
        HistoryTable* t_minutes = new HistoryTable( "Minutes" );
        HistoryTable* t_hours = new HistoryTable( "Hours" );
        HistoryTable* t_days = new HistoryTable( "Days" );

        t_recent->setExpiry( (time_t)(2*60) );
        t_recent->setWindow( (time_t)60 );
        t_recent->cascade( (time_t)60, t_minutes );

        t_minutes->setExpiry( (time_t)(2*60*60) );
        t_minutes->setWindow( (time_t)(1*60*60) );
        t_minutes->cascade( (time_t)(1*60*60), t_hours );

        t_hours->setExpiry( (time_t)(48*60*60) );
        t_hours->setWindow( (time_t)(24*60*60) );
        t_hours->cascade( (time_t)(24*60*60), t_days );

        t_days->setExpiry( (time_t)(60*24*60*60) );
        t_days->setWindow( (time_t)(30*24*60*60) );
        
        int rport = args->getOptionAsInt( "rp" );
        const char* raddr = args->getOptionAsString( "ra" );

        while ( Task::shouldQuit() == 0 ) {
            /* process */
            int num_devices = args->countList();

            for ( int i = 0; i < num_devices; i++ ) {

                ModBus::Value   value[3];
                int id = args->getListItemAsInt( i );

                log( LOG_INFO, "------- read device %d at %s:%d-------", id, raddr, rport );

                ModBus modbus( raddr, rport, id );

                time_t  now;
                struct tm* tm;
                char timestamp[64];
                (void)time(&now);
                tm = localtime( &now );
                snprintf( timestamp, sizeof(timestamp), "%04d-%02d-%02d %02d:%02d:%02d",
                                tm->tm_year+1900,
                                tm->tm_mon+1,
                                tm->tm_mday,
                                tm->tm_hour,
                                tm->tm_min,
                                tm->tm_sec);

                (void)modbus.readVariable( Renogy::PV_INPUT_VOLTAGE, 100, 1, value ); 
                float input_voltage = value[0].asFloat();
                (void)modbus.readVariable( Renogy::PV_INPUT_CURRENT, 100, 1, value ); 
                float input_current = value[0].asFloat();

                t_recent->addRecord( time(NULL), id, input_voltage, input_current );
            }
        }
        http.join();

        delete t_days;
        delete t_hours;
        delete t_minutes;
        delete t_recent;
        return 0;
    }
};

ENTRYPOINT( History )
DEFAULT_ARGS(   "-ra:IP address of remote serial port controller:127.0.0.1"
                "-rp:TCP port for serial controller:32700"
                "-p:TCP port to listen on:33000" 
                "-f:Database file:history.db"
                "--:list of charge controller IDs to work on" )



