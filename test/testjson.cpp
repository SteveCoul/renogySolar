
#include <Common.hpp>
#include <XML2JSON.hpp>


class Test {
public:
static
int main( Args* args ) {
    std::string xml;

    xml+="<root attribute1=one attribute2=2>\n";
    xml+="</root>";

    XML2JSON j;

    std::string json = j.convert( xml );

    std::cout << xml;
    std::cout << json;
    return 0;
}

};

ENTRYPOINT(Test)
DEFAULT_ARGS( "" )

