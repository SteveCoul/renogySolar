
#include <Common.hpp>
#include <HTTPServer.hpp>

class Test : public HTTPServerImplementation {
public:

    int HTTPServerRequest( std::string path, std::string query, std::string& response, std::string& content_type, std::string& body ) {
        body = "<HTML><BODY>You requested and got " + path + "</BODY></HTML>";
        content_type = "text/html";
        response = "Okay";
        return 200;
    }

static
int main( Args* args ) {
    HTTPServer r;
    Test t;
    r.setHandler( &t );

    while (Common::shouldQuit() == 0 ) {        
        r.process();
    }

    return 0;
}

};

ENTRYPOINT(Test)
DEFAULT_ARGS( "-p:TCP port to listen on:8080" )

