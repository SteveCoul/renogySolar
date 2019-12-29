#ifndef __XML2JSON_hpp__
#define __XML2JSON_hpp__

#include <iostream>
#include <sstream>
#include <vector>

#include "XMLParser.hpp"

class XML2JSON : public XMLParser {
public:

    class Item {
    public:
        std::string             name;
        std::string             value;
        class Item*             parent;
        std::vector<class Item*>attributes;
        std::vector<class Item*>children;
        Item( class Item* parent, std::string name, std::string value = "" );
        static bool compare2( Item* one, Item* two );
    };

    void dodel( XML2JSON::Item* i );
    ~XML2JSON();

    XML2JSON::Item*   tree = 0;
    XML2JSON::Item*   current = 0;
    std::stringstream   json;

    void walk( XML2JSON::Item* i, bool is_last = false );
    void onStartTag( std::string name );
    void onEndTag( std::string name );
    void onAttribute( std::string name, std::string value );
    void onText( std::string text );
    std::string onFinish();

    std::string convert( std::string xml );
};

#endif

