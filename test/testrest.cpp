
#include <Common.hpp>
#include <HTTPREST.hpp>

class Test {
public:
static
int main( Args* args ) {
    HTTPREST r;
        
    r.run();

    return 0;
}

};

ENTRYPOINT(Test)
DEFAULT_ARGS( "-p:TCP port to listen on:8080" )

