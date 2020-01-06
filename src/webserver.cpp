
/* worlds stupidest and most dangerous webserver */

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

#include "Common.hpp"
#include "Log.hpp"
#include <HTTPServer.hpp>

class WebServer : public HTTPServerImplementation {
public:

		std::string loadTextFile( std::string path ) {
			std::string r;
			int fd = open( path.c_str(), O_RDONLY );
			if ( fd >= 0 ) {
				char buffer[1024];
				size_t len;
				while ( (len=read(fd,buffer,sizeof(buffer))) > 0 ) {
					r.append( buffer, len );
				}
				close( fd );
			}
			return r;
		}

    int HTTPServerRequest( std::string path, std::string query, std::string& response, std::string& content_type, std::string& body ) {
        if ( path.compare("/")==0 ) path = "/index.html";
        body = loadTextFile( "www" + path );
        if ( body.size() > 0 ) {
            response = "OK";
            return 200;
        }
        response = "Not Found";
        return 404;
    }

    static
    int main( Args* args ) {

        int myport = args->getOptionAsInt( "p" );
        HTTPServer server( myport );
        WebServer w;
        
        server.setHandler( &w );
        while (Common::shouldQuit() == 0 ) {        
            server.process();
        }

        return 0;
    }
};

ENTRYPOINT( WebServer )
DEFAULT_ARGS( "-p:TCP port to listen on:8080" )

