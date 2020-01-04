
#include <unistd.h>

#include <Common.hpp>

#include <HTTPREST.hpp>

class DefaultHandler : public HTTPRESTImplementation {
public:
    int HTTPRESTRequest( std::string path, std::string query, std::string& content_type, std::string& body ) {
        log( LOG_WARNING, "Default HTTP Request Handler invoked:" );
        log( LOG_WARNING, "  returning 404" );
        return 404;
    }
};

HTTPREST::HTTPREST( int port ) {
    m_default_handler = new DefaultHandler();
    m_handler = m_default_handler;
    m_port = port;
}

HTTPREST::~HTTPREST() {
    delete m_handler;
}

void HTTPREST::setHandler( HTTPRESTImplementation* who ) {
    if ( who == NULL ) who = m_default_handler;
    m_handler = who;
}

void HTTPREST::run() {
    log( LOG_NOTICE, "HTTPREST running...." );

    int fd = Common::createTCPServerSocket( m_port );
    if ( fd >= 0 ) {
        for (;;) {
            int client = Common::tcpAccept( fd );
            if ( client < 0 ) break;

            log( LOG_ERR, "TODO - client connection for %d", client );
            close( client );
        }
    }

    close( fd );

    log( LOG_NOTICE, "HTTPREST closing...." );
}

