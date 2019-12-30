#include <algorithm>
#include "XML2JSON.hpp"

XML2JSON::XML2JSON() {
	tree = 0;
	current = 0;
}

void XML2JSON::Item::sort() {
    std::vector<class Item*>s(children);

    children.clear();

    while ( s.empty() == false ) {
        Item* i = s.front();
        s.erase( s.begin() );

        std::vector<Item*>::iterator it = children.begin();
        while ( it != children.end() ) {
            if ( (*it)->name.compare( i->name ) == 0 ) break;
            it++;
        }

        if ( it != children.end() ) {
            while ( it != children.end() ) {
                if ( (*it)->name.compare( i->name ) != 0 ) break;
                it++;
            }
        }

        children.insert( it, i );
    }
}

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

void XML2JSON::walk( XML2JSON::Item* i, bool is_last, bool is_array_member ) {

    if ( ( i->children.size() == 1 ) && ( i->children.at(0)->name.compare("#text")==0 ) ) {
        json << i->name << ": " << i->children.at(0)->value;
        if ( !is_last ) json << ",";
        json << "\n";
    } else {

        if ( is_array_member ) {
            json << "{\n";
        } else {
           json << i->name << ": {\n";
        }

        i->sort();

        for (std::vector<Item*>::iterator it = i->attributes.begin() ; it != i->attributes.end(); ++it) {
            json << (*it)->name << ": " << (*it)->value;
            if ( ( i->children.size() > 0 ) || ((it+1) != i->attributes.end() ) )
                json << ",";
            json << "\n";
        }

        std::vector<Item*>::iterator it = i->children.begin();
        bool array = false;

        for (;;) {
            Item* here = *it;
            bool ending = false;
            if ( array == false ) {
                if ( here == i->children.back() ) {
                    /* last element, so we can't be */
                } else {
                    Item* there = *(it+1);
                    if ( here->name.compare( there->name ) == 0 ) {
                        /* two names match, starting an array */
                        array = true;
                        json << here->name << ": [\n";
                    }
                }
            } else if ( array ) {
                if ( here == i->children.back() ) {
                    /* last child so must be end of array */
                    ending = true;
                } else {
                    Item* there = *(it+1);
                    if ( here->name.compare( there->name ) != 0 ) {
                        /* name changes */
                        ending = true;
                    }
                }
            }
           
            if ( array ) {
                walk(*it, ending, true );
            } else {
                walk(*it, (it+1)==i->children.end(), false );
            }

            if ( ending ) {
                if ( here != i->children.back() ) json << "],\n";
                else json << "]\n";
                array = false;
            }

            it++;
            if ( it == i->children.end() ) break;
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

std::string XML2JSON::convert( std::string xml ) {
    if ( parse( xml ) ) return result();
    return "#error";
}

