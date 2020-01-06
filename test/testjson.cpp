
#include <Task.hpp>
#include <XML2JSON.hpp>


class Test {
public:
static
int main( Args* args ) {
    std::string xml;

    xml+="<root attribute1=one attribute2=2>\n";
    xml+="  <partial_distributed_array>\n";
    xml+="      <not_array_member1>not array</not_array_member1>\n";
    xml+="      <array_member id=1><array_child>child</array_child></array_member>\n";
    xml+="      <array_member id=2><array_child>child</array_child></array_member>\n";
    xml+="      <array_member id=3><array_child>child</array_child></array_member>\n";
    xml+="      <not_array_member2>not array</not_array_member2>\n";
    xml+="      <array_member id=4><array_child>child</array_child></array_member>\n";
    xml+="      <array_member id=5><array_child>child</array_child></array_member>\n";
    xml+="      <not_array_member3>not array</not_array_member3>\n";
    xml+="      <not_array_member4>not array</not_array_member4>\n";
    xml+="  </partial_distributed_array>\n";
    xml+="</root>";

    XML2JSON j;

    std::string json = j.convert( xml );

    std::cout << xml;
    std::cout << "--------------------------------------------------------\n";
    std::cout << json;
    return 0;
}

};

ENTRYPOINT(Test)
DEFAULT_ARGS( "" )

