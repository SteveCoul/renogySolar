
#include <unistd.h>

#include <Common.hpp>

#include <HTTPServer.hpp>

class DefaultHandler : public HTTPServerImplementation {
public:
    int HTTPServerRequest( std::string path, std::string query, std::string& content_type, std::string& body ) {
        log( LOG_WARNING, "Default HTTP Request Handler invoked:" );
        log( LOG_WARNING, "  returning 404" );
        return 404;
    }
};

HTTPServer::HTTPServer( int port ) {
    m_default_handler = new DefaultHandler();
    m_handler = m_default_handler;
    m_port = port;
    m_fd = Common::createTCPServerSocket( m_port );
}

HTTPServer::~HTTPServer() {
    delete m_handler;
    close( m_fd );
}

void HTTPServer::setHandler( HTTPServerImplementation* who ) {
    if ( who == NULL ) who = m_default_handler;
    m_handler = who;
}

void HTTPServer::process( int timeout ) {

    if ( timeout >= 0 ) log( LOG_CRIT, "TIMEOUT NOT IMPLEMENTED FOR HTTP SERVER YET" );

    int client = Common::tcpAccept( m_fd );
    if ( client < 0 ) return;
    
    std::vector<std::string> request;
    for (;;) {
        std::string str;
        int ret = Common::readLine( client, str );
        if ( ret < 0 ) {
            log( LOG_CRIT, "Error reading http request and I dont handle it very well" );
            break;
        }
        if ( str.empty() ) break;
    }

    log( LOG_ERR, "TODO - client connection for %d", client );
}

