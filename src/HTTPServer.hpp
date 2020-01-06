#ifndef __HTTPServer_hpp__
#define __HTTPServer_hpp__

#include <string>

class HTTPServerImplementation {
public:
    virtual ~HTTPServerImplementation() { }
    virtual int HTTPServerRequest( std::string path, std::string query, std::string& response, std::string& content_type, std::string& body ) = 0;
};

class HTTPServer {
public:
	HTTPServer( int port = 8080 );
	~HTTPServer();
    void setHandler( HTTPServerImplementation* who );
    void process( int timeout = -1 );
private:
    static int readLine( int fd, std::string& result );
    static std::vector<std::string> tokenizeString( std::string source, std::string dividers );
    static std::string toUpper( std::string source );
private:
    int                             m_fd;
    int                         m_port;
    HTTPServerImplementation*     m_default_handler;
    HTTPServerImplementation*     m_handler;
};

#endif

