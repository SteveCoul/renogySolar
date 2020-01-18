#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <sys/poll.h>

#include <algorithm>
#include <sstream>
#include <vector>

#include "Log.hpp"
#include <TCP.hpp>

#include <HTTPServer.hpp>

class DefaultHandler : public HTTPServerImplementation {
public:
    int HTTPServerRequest( std::string path, std::string query, std::string& response, std::string& content_type, std::string& body ) {
        log( LOG_WARNING, "Default HTTP Request Handler invoked:" );
        log( LOG_WARNING, "  returning 404" );
        response = "Not Found";
        return 404;
    }
};

HTTPServer::HTTPServer( int port ) {
    m_default_handler = new DefaultHandler();
    m_handler = m_default_handler;
    m_port = port;
    m_fd = TCP::createServerSocket( m_port );
}

HTTPServer::~HTTPServer() {
    delete m_default_handler;
    close( m_fd );
}

void HTTPServer::setHandler( HTTPServerImplementation* who ) {
    if ( who == NULL ) who = m_default_handler;
    m_handler = who;
}

void HTTPServer::process( int timeout ) {

    if ( timeout >= 0 ) log( LOG_CRIT, "TIMEOUT NOT IMPLEMENTED FOR HTTP SERVER YET" );

    int client = TCP::accept( m_fd );
    if ( client < 0 ) return;

    log( LOG_INFO, "Incoming HTTP Request" );    
    std::vector<std::string> request;
    for (;;) {
        std::string str;
        int ret = readLine( client, str );
        if ( ret < 0 ) {
            log( LOG_CRIT, "Error reading http request and I dont handle it very well" );
            break;
        }
        log( LOG_INFO, "> %s", str.c_str() );
        if ( str.empty() ) break;
        request.push_back( str );
    }

    std::string url;
    /* pretty dumb server - just look for the GET line, don't care about anything else at this point */
    for( std::vector<std::string>::iterator it = request.begin(); it != request.end(); it++ ) {
        std::vector<std::string> tokens = tokenizeString( (*it), " \t" );
        if ( tokens.size() >= 3 ) {
            if ( toUpper( tokens.at(0) ).compare( "GET" ) == 0 ) {
                url = tokens.at(1);
                break;
            }
        }
    }

    int http_code;
    std::string http_response;
    std::string content_type;
    std::string body;

    if ( url.empty() ) {
        log( LOG_WARNING, "didn't find a GET in the request" );
        http_code = 500;
        http_response = "Server Error";
    } else {
        log( LOG_INFO, "Looks like a GET for '%s'", url.c_str() );
        std::vector<std::string> split = tokenizeString( url, "?" );

        std::string path = split.at(0);
        std::string query;
        if ( split.size() > 1 ) query = split.at(1);

        http_code = m_handler->HTTPServerRequest( path, query, http_response, content_type, body );
    }

    std::stringstream out;

    out << "HTTP/1.0 " << http_code << " " << http_response << "\r\n";
    out << "Access-Control-Allow-Origin: *\r\n";
    out << "Content-Length: " << body.size() << "\r\n";
    out << "Connection: close\r\n";
    if ( !content_type.empty() ) out << "Content-Type: " << content_type << "\r\n";
    out << "\r\n";
    if ( body.size() > 0 ) out << body;

    std::string body_string = out.str();
    const char* p = body_string.c_str();
    size_t len = strlen(p);

    log( LOG_INFO, "%s", p );
    (void)write( client, p, len );
    TCP::waitForHangup( client );
    (void)close( client );
}

int HTTPServer::readLine( int fd, std::string& result ) {
    result = "";
    for (;;) {
        int ret;
        struct pollfd pfd;
        memset( &pfd, 0, sizeof(pfd) );
        pfd.fd = fd;
        pfd.events = POLLIN;

        ret = poll( &pfd, 1, 1000 );
        if ( ret < 0 ) {
            log( LOG_WARNING, "failed poll in readline [%s]", strerror(errno) );
            return -1;
        }

        if ( ( pfd.revents & POLLIN ) == 0 ) {
            log( LOG_WARNING, "no data, timeout on readline in http server [%s]", strerror(errno) );
            errno = ETIMEDOUT;
            return -1;
        }

        char c;
        ret = read( fd, &c, 1 );
        if ( ret == 0 ) 
            break;
        else if ( ret < 0 ) {
            log( LOG_WARNING, "error in read() [%s]", strerror(errno) );
            return -1;
        }

        if ( c == '\r' ) {
            /* ignore */
        } else if ( c == '\n' ) {
            break;
        } else {
            result += c;
        }
    }
    return result.length();
}

std::vector<std::string> HTTPServer::tokenizeString( std::string source, std::string dividers ) {
    std::vector<std::string> tokens;
    for (;;) {
        size_t where;
        while ( ( where = source.find_first_of( dividers ) ) == 0 ) 
            source.erase(0,1);
        if ( source.empty() ) break;
        if ( where == std::string::npos ) {
            tokens.push_back( source );
            break;
        }
        tokens.push_back( source.substr( 0, where ) );
        source.erase( 0, where );
    }
    return tokens;
}

std::string HTTPServer::toUpper( std::string source ) {
    std::string ret;
    std::for_each( source.begin(), source.end(), [&]( char const &c ) {
        ret+=std::toupper( c );
    } );
    return ret;
}

