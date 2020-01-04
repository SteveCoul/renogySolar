
#include <Common.hpp>
#include <HTTPServer.hpp>

class Test {
public:
static
int main( Args* args ) {
    HTTPServer r;

    while (Common::shouldQuit() == 0 ) {        
        r.process();
    }

    return 0;
}

};

ENTRYPOINT(Test)
DEFAULT_ARGS( "-p:TCP port to listen on:8080" )

