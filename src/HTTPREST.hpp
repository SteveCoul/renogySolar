#ifndef __HTTPREST_hpp__
#define __HTTPREST_hpp__

#include <string>

class HTTPRESTImplementation {
public:
    virtual ~HTTPRESTImplementation() { }
    virtual int HTTPRESTRequest( std::string path, std::string query, std::string& content_type, std::string& body ) = 0;
};

class HTTPREST {
public:
	HTTPREST( int port = 8080 );
	~HTTPREST();
    void setHandler( HTTPRESTImplementation* who );
    void run();
private:
    int                         m_port;
    HTTPRESTImplementation*     m_default_handler;
    HTTPRESTImplementation*     m_handler;
};

#endif

