
#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <malloc.h>
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

#include <sqlite3.h>

#include "common.hpp"
#include "modbus.hpp"
#include "renogy.hpp"

int main( int argc, char** argv ) {

	int rport = atoi( argv[2] );
	const char* raddr = argv[1];
	const char* database = argv[3];
	/* args 4..argc are id's to read and log*/
	int rc;
	sqlite3* db;
	sqlite3_stmt* statement;

	rc = sqlite3_open( database, &db );
	if ( rc != SQLITE_OK ) {
		fprintf( stderr, "Failed to open database\n");
		assert(0);
	}

	const char* sql = 	"DROP TABLE IF EXISTS Recent;"
						"CREATE TABLE Recent( id INT, timestamp DATETIME, input_voltage FLOAT(24), input_current FLOAT(24) );"
						"DROP TABLE IF EXISTS Minutes;"
						"CREATE TABLE Minutes( id INT, timestamp DATETIME, input_voltage FLOAT(24), input_current FLOAT(24) );";

	char *err_msg;

	rc = sqlite3_exec( db, sql, 0, 0, &err_msg );
	if ( rc != SQLITE_OK ) {
		fprintf( stderr, "error %s\n", err_msg );
		sqlite3_free( err_msg );
		sqlite3_close( db );
		assert(0);
	}

	for (;;) {
		sleep(1);
		for ( int i = 4; i < argc; i++ ) {
			modbus_data_value_t value[3];
			int id = atoi( argv[i] );

			printf("Read %d\n", id );

			time_t	now;
			struct tm* tm;
			char timestamp[64];
			char timestamp2[64];
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

			char command[1024];

			/* add current record to recent */
			snprintf( command, sizeof(command), "INSERT INTO Recent VALUES ( %d, '%s', %f, %f );", id, timestamp, input_voltage, input_current );
			printf("%s\n", command );
			if ( sqlite3_exec( db, command, 0, 0, &err_msg ) != SQLITE_OK ) {
				fprintf( stderr, "error %s\n", err_msg );
				sqlite3_free( err_msg );
			}

			/* trim recent table so nothing older than say 2 minutes */

			(void)time(&now);
			now = now - 120;
			tm = localtime( &now );
			snprintf( timestamp, sizeof(timestamp), "%04d-%02d-%02d %02d:%02d:%02d",
							tm->tm_year+1900,
							tm->tm_mon+1,
							tm->tm_mday,
							tm->tm_hour,
							tm->tm_min,
							tm->tm_sec);
			snprintf( command, sizeof(command), "DELETE FROM Recent WHERE timestamp<'%s' AND ID=%d;", timestamp, id );
			printf("%s\n", command );
			if ( sqlite3_exec( db, command, 0, 0, &err_msg ) != SQLITE_OK ) {
				fprintf( stderr, "error %s\n", err_msg );
				sqlite3_free( err_msg );
			}

			/* select everything from the current minute and make some values */
			(void)time(&now);
			tm = localtime( &now );
			snprintf( timestamp, sizeof(timestamp), "%04d-%02d-%02d %02d:%02d:%02d",
							tm->tm_year+1900,
							tm->tm_mon+1,
							tm->tm_mday,
							tm->tm_hour,
							tm->tm_min,
							0 );
			snprintf( timestamp2, sizeof(timestamp), "%04d-%02d-%02d %02d:%02d:%02d",
							tm->tm_year+1900,
							tm->tm_mon+1,
							tm->tm_mday,
							tm->tm_hour,
							tm->tm_min,
							59 );
			snprintf(command,sizeof(command), "SELECT * FROM Recent WHERE timestamp>=? AND timestamp<=? AND id=?;");

			printf("%s (%s,%s)\n", command, timestamp, timestamp2 );
			if ( sqlite3_prepare( db, command, -1, &statement, NULL ) != SQLITE_OK ) {
				fprintf( stderr, "error\n" );
			}

			(void)sqlite3_reset( statement );

			if ( sqlite3_bind_text( statement, 1, timestamp, strlen(timestamp), SQLITE_STATIC ) != SQLITE_OK ) {
				assert(0);
			}

			if ( sqlite3_bind_text( statement, 2, timestamp2, strlen(timestamp2)+1, 0 ) != SQLITE_OK ) {
				assert(0);
			}

			if ( sqlite3_bind_int( statement, 3, id ) != SQLITE_OK ) {
				assert(0);
			}

			float total_voltage = 0.0f;
			float total_current = 0.0f;
			int count = 0;
			while ( sqlite3_step(statement) == SQLITE_ROW ) {
				total_voltage += (float)sqlite3_column_double( statement, 2 );
				total_current += (float)sqlite3_column_double( statement, 3 );
				count++;
			}
			total_voltage /= count;
			total_current /= count;

			sqlite3_finalize(statement);

			/* Write the averages to the minutes database */

			/* I wanted to do INSERT or UPDATE, but I couldn't get count to work so I could tell which to do! */

			snprintf( command, sizeof(command), "DELETE FROM Minutes WHERE timestamp='%s' AND ID=%d;", timestamp, id );
			printf("%s\n", command );
			if ( sqlite3_exec( db, command, 0, 0, &err_msg ) != SQLITE_OK ) {
				fprintf( stderr, "error %s\n", err_msg );
				sqlite3_free( err_msg );
			}
			snprintf( command, sizeof(command), "INSERT INTO Minutes VALUES( %d, '%s', %f, %f );", id, timestamp, total_voltage, total_current );
			printf("%s\n", command );
			if ( sqlite3_exec( db, command, 0, 0, &err_msg ) != SQLITE_OK ) {
				fprintf( stderr, "error %s\n", err_msg );
				sqlite3_free( err_msg );
			}

			/* Drop from the minute, by minute table anything over an hour old */
			(void)time(&now);
			now = now - 3600;
			tm = localtime( &now );
			snprintf( timestamp, sizeof(timestamp), "%04d-%02d-%02d %02d:%02d:%02d",
							tm->tm_year+1900,
							tm->tm_mon+1,
							tm->tm_mday,
							tm->tm_hour,
							tm->tm_min,
							tm->tm_sec);
			snprintf( command, sizeof(command), "DELETE FROM Minutes WHERE timestamp<'%s' AND ID=%d;", timestamp, id );
			printf("%s\n", command );
			if ( sqlite3_exec( db, command, 0, 0, &err_msg ) != SQLITE_OK ) {
				fprintf( stderr, "error %s\n", err_msg );
				sqlite3_free( err_msg );
			}

			/* select everything from the minutes table that matches the current hour, average and store in hour table */
			/* wipe everything after 168 hours from the hours table ( one week history hourly ) */

			/* consider a daily average table and wipe after say 10 years? */

			/* Then we need a webserver to query, xml-ize and output each table */
		}
	}
}

