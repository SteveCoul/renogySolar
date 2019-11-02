
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

#include "Common.hpp"
#include "modbus.hpp"
#include "renogy.hpp"

///
/// \class Test
///
/// \brief 	A Task to perform a single modbus read operation for testing.
///
class Test {
public:

	static
	int main( int argc, char** argv ) {
	
		// FIXME proper arg parsing
		int rport = atoi( argv[2] );
		const char* raddr = argv[1];
		int id = atoi( argv[3] );
	
		signal( SIGPIPE, SIG_IGN );
		modbus_data_value_t value[1];
	
		(void)modbusReadVariable( raddr, rport, id, RENOGY_RATED_INPUT_VOLTAGE, 100, 1, value ); 
		return 0;
	}

};

ENTRYPOINT( Test )

