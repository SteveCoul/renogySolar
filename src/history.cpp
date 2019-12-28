
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

#include "Common.hpp"
#include "HistoryTable.hpp"
#include "ModBus.hpp"
#include "renogy.hpp"

class History {
public:
static
int main( Args* args ) {
    int rport = args->getOptionAsInt( "rp" );
    const char* raddr = args->getOptionAsString( "ra" );

    int server = Common::createTCPServerSocket( args->getOptionAsInt( "p" ) );

    HistoryTable::cinit( args->getOptionAsString("f") );

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
    

    while ( Common::shouldQuit() == 0 ) {
        struct pollfd pfd;
        memset( &pfd, 0, sizeof(pfd) );
        pfd.fd = server;
        pfd.events = POLLIN;
        (void)poll( &pfd, 1, 1000 );
        if ( pfd.revents ) {
            char buffer[1024];
            int client_fd = Common::tcpAccept( server );
            char* line = Common::mReadLine( client_fd );

            int i = 0;
            while( ( line[i] != ' ' ) && ( line[i] != '\t' ) ) i++;
            while( ( line[i] == ' ' ) || ( line[i] == '\t' ) ) i++;
            while( line[i] == '/' ) i++;
    
            char* clientid = line+i;
            char* name = clientid;
            while( name[0] != '/' ) name++;
            name[0] = '\0';
            name++;

            i = 0;
            while ( ( name[i] != '\0' ) && ( name[i] != '?' ) && ( name[i] != ' ' ) && ( name[i] != '\t' ) )    i++;
            name[i] = 0;
            while ( i!=0 ) {
                if ( name[i-1] != '.' ) {
                    i--;
                    name[i] = 0;
                } else {
                    break;
                }
            }
            name[i-1] = 0;

            HistoryTable* tab = HistoryTable::get( name );
            if ( tab == NULL ) {
                sprintf( buffer, "HTTP/1.0 404 Not Found\r\nAccess-Control-Allow-Origin: *\r\nContent-Length: 0\r\n\r\n" );
                write( client_fd, buffer, strlen(buffer) );
            } else {
                char* xml = tab->toXML( atoi(clientid) );
                sprintf( buffer, "HTTP/1.0 200 Okay\r\nAccess-Control-Allow-Origin: *\r\nContent-Length: %d\r\n\r\n", (int)strlen(xml) );
                write( client_fd, buffer, strlen(buffer) );
                write( client_fd, xml, strlen(xml) );
                free( (void*)xml );
            }

            free( (void*)line );
            Common::waitForTCPHangup( client_fd );

            close( client_fd );
        }

        /* process */
        int num_devices = args->countList();

        for ( int i = 0; i < num_devices; i++ ) {

            ModBus::Value   value[3];
            int id = args->getListItemAsInt( i );

            log( LOG_INFO, "------- read device %d -------", id );

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

            (void)modbus.readVariable( RENOGY_PV_INPUT_VOLTAGE, 100, 1, value ); 
            float input_voltage = value[0].asFloat();
            (void)modbus.readVariable( RENOGY_PV_INPUT_CURRENT, 100, 1, value ); 
            float input_current = value[0].asFloat();

            t_recent->addRecord( time(NULL), id, input_voltage, input_current );
        }
    }
    return 0;
}
};

ENTRYPOINT( History )
DEFAULT_ARGS(   "-ra:IP address of remote serial port controller:127.0.0.1"
                "-rp:TCP port for serial controller:32700"
                "-p:TCP port to listen on:33000" 
                "-f:Database file:history.db"
                "--:list of charge controller IDs to work on" )



