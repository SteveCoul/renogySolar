
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
#include "modbus.hpp"
#include "renogy.hpp"

class Table;
typedef void (Table::*callback_fn)( void* param, const char* time, float voltage, float current );

class Table {
public:
	struct averaging {
		int count;
		float voltage;	
		float current;
	};
public:
	Table( const char* name )	: m_window(1)
								, m_expiry(1)
								, m_cascade_time(1)
								, m_cascade_target(0)
								{
		char c[128];
		m_name = strdup( name );
		snprintf( c, sizeof(c), "CREATE TABLE IF NOT EXISTS %s( id INT, timestamp DATETIME, input_voltage FLOAT(24), input_current FLOAT(24) );", name );
		sqlExec( c );

		snprintf(c,sizeof(c), "SELECT * FROM %s WHERE timestamp>=? AND timestamp<=? AND id=?;", m_name);

		if ( sqlite3_prepare( c_db, c, -1, &m_for_each_in_range, NULL ) != SQLITE_OK ) {
			log( LOG_ERR, "Failed prepare" );
		}

		m_next = c_list;
		c_list = this;
	}

	~Table() {
		free( (void*)m_name );
		sqlite3_finalize(m_for_each_in_range);
	}

	void addRecord( time_t now, int id, float input_voltage, float input_current ) {
		char timestamp[64];
		mktimestamp( timestamp, now );
		log( LOG_INFO, "%s->%s( %s, %d, %f, %f )\n", m_name, __FUNCTION__, timestamp, id, input_voltage, input_current );
	
		char c[512];
		snprintf( c, sizeof(c), "DELETE FROM %s WHERE id=%d and timestamp='%s';", m_name, id, timestamp );
		sqlExec( c );

		snprintf( c, sizeof(c), "INSERT INTO %s VALUES ( %d, '%s', %f, %f );", m_name, id, timestamp, input_voltage, input_current );
		sqlExec( c );

		mktimestamp( timestamp, now - m_expiry );
		snprintf( c, sizeof(c), "DELETE FROM %s WHERE id=%d and timestamp<'%s';", m_name, id, timestamp );
		sqlExec( c );

		if ( m_cascade_target ) {
			time_t new_time = ( now / m_cascade_time ) * m_cascade_time;
			char start_time[64];
			mktimestamp( start_time, new_time );
			char end_time[64];
			mktimestamp( end_time, new_time + m_cascade_time - 1 );
			log( LOG_INFO, "Get averages from %s to %s\n", start_time, end_time );

			struct averaging av;
			av.count = 0;
			av.voltage = 0.0f;
			av.current = 0.0f;

			forEachInTimeRange( id, start_time, end_time, &Table::doAveraging, &av );

			m_cascade_target->addRecord( new_time, id, av.voltage / av.count, av.current / av.count );

		}
	}

	void setWindow( time_t t ) {
		m_window = t;
	}

	void setExpiry( time_t t ) {
		m_expiry = t;
	}

	void cascade( time_t time_mod, class Table* to ) {
		m_cascade_time = time_mod;
		m_cascade_target = to;
	}

	char* toXML( int id ) {
		char* result = NULL;

		char start_time[64];
		mktimestamp( start_time, time(NULL) - m_window );

		char end_time[64];
		mktimestamp( end_time, time(NULL) );

		Common::mprintf( &result, "<xml>\n" );
		forEachInTimeRange( id, start_time, end_time, &Table::toXMLcallback, &result );
		Common::mprintf( &result, "</xml>\n" );
		return result;
	}

private:

	void doAveraging( void* param, const char* t, float voltage, float current ) {
		struct averaging* p = (struct averaging*)param;
		p->count++;
		p->voltage += voltage;
		p->current += current;
	}

	void toXMLcallback( void* param, const char* t, float voltage, float current ) {
		Common::mprintf( (char**)param, "<entry time='%s'>\n", t );
		Common::mprintf( (char**)param, "<voltage>%f</voltage>\n", voltage );
		Common::mprintf( (char**)param, "<current>%f</current>\n", current );
		Common::mprintf( (char**)param, "</entry>\n");
	}

	void forEachInTimeRange( int id, const char* start_time, const char* end_time, callback_fn callback, void* param ) {

		sqlite3_reset( m_for_each_in_range );

		if ( sqlite3_bind_text( m_for_each_in_range, 1, start_time, strlen( start_time ), SQLITE_STATIC ) != SQLITE_OK ) {
			assert(0);
		}

		if ( sqlite3_bind_text( m_for_each_in_range, 2, end_time, strlen( end_time), 0 ) != SQLITE_OK ) {
			assert(0);
		}

		if ( sqlite3_bind_int( m_for_each_in_range, 3, id ) != SQLITE_OK ) {
			assert(0);
		}

		while ( sqlite3_step(m_for_each_in_range) == SQLITE_ROW ) {
			(this->*callback)(  param, 
								(const char*)sqlite3_column_text( m_for_each_in_range, 1 ),
							    (float)sqlite3_column_double( m_for_each_in_range, 2 ),
							    (float)sqlite3_column_double( m_for_each_in_range, 3 ) );
		}
	}

private:
	const char*		m_name;
	time_t			m_window;
	time_t			m_expiry;
	time_t			m_cascade_time;
	class Table*	m_cascade_target;
	class Table*	m_next;
	sqlite3_stmt*	m_for_each_in_range;
public:

	static
	Table* get( const char* name ) {
		Table* head = c_list;
		while ( head != NULL ) {
			if ( strcmp( name, head->m_name ) == 0 ) break;
			head=head->m_next;
		}
		return head;
	}

	static
	void mktimestamp( char* timestamp, time_t now ) {
		struct tm* tm;
		tm = localtime( &now );
		sprintf( timestamp, "%04d-%02d-%02d %02d:%02d:%02d",
							tm->tm_year+1900,
							tm->tm_mon+1,
							tm->tm_mday,
							tm->tm_hour,
							tm->tm_min,
							tm->tm_sec);
	}

	static
	void sqlExec( const char* command ) {
		char *err_msg;

		log( LOG_DEBUG, command );
		int rc = sqlite3_exec( c_db, command, 0, 0, &err_msg );
		if ( rc != SQLITE_OK ) {
			fprintf( stderr, "error %s\n", err_msg );
			sqlite3_free( err_msg );
			assert(0);
		}
	}

	static
	int cinit( const char* database ) {
		if ( sqlite3_open( database, &c_db ) != SQLITE_OK ) {
			log( LOG_CRIT, "Failed to open database" );
			return -1;
		}
		return 0;
	}

private:
	static	sqlite3*		c_db;
	static class Table*		c_list;
};
sqlite3*		Table::c_db;
class Table*	Table::c_list = 0;

class History {
public:
static
int main( int argc, char** argv ) {
	int rport = atoi( argv[2] );
	const char* raddr = argv[1];
	/* args 4..argc are id's to read and log*/

	int server = Common::createTCPServerSocket( atoi( argv[3] ) );

	openlog( NULL, LOG_PID, LOG_USER );

	Table::cinit( argv[4] );

	Table* t_recent = new Table( "Recent" );
	Table* t_minutes = new Table( "Minutes" );
	Table* t_hours = new Table( "Hours" );
	
	t_recent->setExpiry( (time_t)(2*60) );
	t_recent->setWindow( (time_t)60 );
	t_recent->cascade( (time_t)60, t_minutes );

	t_minutes->setExpiry( (time_t)(2*60*60) );
	t_minutes->setWindow( (time_t)(1*60*60) );
	t_minutes->cascade( (time_t)(1*60*60), t_hours );

	t_hours->setExpiry( (time_t)(48*60*60) );
	t_hours->setWindow( (time_t)(24*60*60) );

	for (;;) {
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
			while ( ( name[i] != '\0' ) && ( name[i] != '?' ) && ( name[i] != ' ' ) && ( name[i] != '\t' ) )	i++;
			name[i] = 0;

			Table* tab = Table::get( name );
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
		for ( int i = 5; i < argc; i++ ) {
			modbus_data_value_t value[3];
			int id = atoi( argv[i] );

			log( LOG_INFO, "------- read device %d -------", id );

			time_t	now;
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

			(void)modbusReadVariable( raddr, rport, id, RENOGY_PV_INPUT_VOLTAGE, 100, 1, value ); 
			float input_voltage = value[0].asFloat;
			(void)modbusReadVariable( raddr, rport, id, RENOGY_PV_INPUT_CURRENT, 100, 1, value ); 
			float input_current = value[0].asFloat;

			t_recent->addRecord( time(NULL), id, input_voltage, input_current );
		}
	}
	return 0;
}
};

ENTRYPOINT( History )

