
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

sqlite3*			HistoryTable::c_db;
class HistoryTable*	HistoryTable::c_list = 0;

HistoryTable::HistoryTable(	const char* name )	: m_window(1)
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

HistoryTable::~HistoryTable() {
	free( (void*)m_name );
	sqlite3_finalize(m_for_each_in_range);
}

void HistoryTable::addRecord( time_t now, int id, float input_voltage, float input_current ) {
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

		forEachInTimeRange( id, start_time, end_time, &HistoryTable::doAveraging, &av );

		if ( av.count > 0 ) {
			av.voltage/=av.count;
			av.current/=av.count;
			m_cascade_target->addRecord( new_time, id, av.voltage, av.current );
		}
	}
}

void HistoryTable::setWindow( time_t t ) {
	m_window = t;
}

void HistoryTable::setExpiry( time_t t ) {
	m_expiry = t;
}

void HistoryTable::cascade( time_t time_mod, class HistoryTable* to ) {
	m_cascade_time = time_mod;
	m_cascade_target = to;
}

char* HistoryTable::toXML( int id ) {
	char* result = NULL;

	char start_time[64];
	mktimestamp( start_time, time(NULL) - m_window );

	char end_time[64];
	mktimestamp( end_time, time(NULL) );

	Common::mprintf( &result, "<xml>\n" );
	forEachInTimeRange( id, start_time, end_time, &HistoryTable::toXMLcallback, &result );
	Common::mprintf( &result, "</xml>\n" );
	return result;
}

void HistoryTable::doAveraging( void* param, const char* t, float voltage, float current ) {
	struct averaging* p = (struct averaging*)param;
	p->count++;
	p->voltage += voltage;
	p->current += current;
}

void HistoryTable::toXMLcallback( void* param, const char* t, float voltage, float current ) {
	Common::mprintf( (char**)param, "<entry time='%s'>\n", t );
	Common::mprintf( (char**)param, "<voltage>%f</voltage>\n", voltage );
	Common::mprintf( (char**)param, "<current>%f</current>\n", current );
	Common::mprintf( (char**)param, "</entry>\n");
}

void HistoryTable::forEachInTimeRange( int id, const char* start_time, const char* end_time, callback_fn callback, void* param ) {

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

HistoryTable* HistoryTable::get( const char* name ) {
	HistoryTable* head = c_list;
	while ( head != NULL ) {
		if ( strcmp( name, head->m_name ) == 0 ) break;
		head=head->m_next;
	}
	return head;
}

void HistoryTable::mktimestamp( char* timestamp, time_t now ) {
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

void HistoryTable::sqlExec( const char* command ) {
	char *err_msg;

	log( LOG_DEBUG, command );
	int rc = sqlite3_exec( c_db, command, 0, 0, &err_msg );
	if ( rc != SQLITE_OK ) {
		log( LOG_ERR, "error %s - processing SQL statement", err_msg );
		sqlite3_free( err_msg );
	}
}

int HistoryTable::cinit( const char* database ) {
	if ( sqlite3_open( database, &c_db ) != SQLITE_OK ) {
		log( LOG_CRIT, "Failed to open database" );
		return -1;
	}
	return 0;
}


