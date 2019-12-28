
#include "XML2JSON.hpp"

XML2JSON::Item::Item( class Item* parent, std::string name, std::string value ) {
    if ( name.front() == '#' ) {
        this->name = name;
    } else {
        this->name = "\"" + name + "\"";
    }
    this->value = value;
    for ( std::string::iterator it = value.begin(); it != value.end(); ++it ) {
        if (!(((*it)=='+')||((*it)=='-')||((*it)=='.')||(((*it)>='0')&&((*it)<='9')))) {
            this->value = "\"" + value + "\"";
            break;
        }
    }
    this->parent = parent;
    if ( parent ) this->parent->children.push_back( this );
}

void XML2JSON::dodel( XML2JSON::Item* t ) {
    for (std::vector<Item*>::iterator it = t->attributes.begin() ; it != t->attributes.end(); ++it)
        delete (*it);
    for (std::vector<Item*>::iterator it = t->children.begin() ; it != t->children.end(); ++it)
        dodel (*it);
    delete t;
}

XML2JSON::~XML2JSON() {
    if ( tree ) {
        dodel( tree );
    }
}

void XML2JSON::walk( XML2JSON::Item* i, bool is_last ) {

    if ( ( i->children.size() == 1 ) && ( i->children.at(0)->name.compare("#text")==0 ) ) {
        json << i->name << ": " << i->children.at(0)->value;
        if ( !is_last ) json << ",";
        json << "\n";
    } else {

        json << i->name << ": {\n";

        for (std::vector<Item*>::iterator it = i->attributes.begin() ; it != i->attributes.end(); ++it) {
            json << (*it)->name << ": " << (*it)->value;
            if ( ( i->children.size() > 0 ) || ((it+1) != i->attributes.end() ) )
                json << ",";
            json << "\n";
        }

        for (std::vector<Item*>::iterator it = i->children.begin() ; it != i->children.end(); ++it) {
            walk(*it, (it+1)==i->children.end() );
        }

        json << "}";
        if ( !is_last ) json << ",";
        json << "\n";
    }
}

std::string XML2JSON::onFinish() { 
    json << "{\n";
    walk( tree, true );
    json << "}\n";
    return json.str();
}

void XML2JSON::onStartTag( std::string name ) { 
    Item* i = new Item( current, name );
    if ( tree == 0 ) tree = i;
    current = i;
}

void XML2JSON::onEndTag( std::string name )   { 
    current = current->parent;
}

void XML2JSON::onAttribute( std::string name, std::string value ) { 
    current->attributes.push_back( new Item( NULL, "@" + name, value ) );
}

void XML2JSON::onText( std::string text )   { 
    (void) new Item( current, "#text", text );
}


