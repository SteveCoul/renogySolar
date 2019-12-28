
#include <iostream>

#include "XMLParser.hpp"

bool XMLParser::isspace( char c ) {
    return ( ( c == ' ' ) || ( c == '\t' ) || ( c == '\r' ) || ( c == '\n' ) );
}

void XMLParser::skipSpace() {
    while ((xml.length()!=0)&&(isspace(xml.front())))
        xml.erase(0,1);
}

void XMLParser::trimSpace( std::string& xml ) {
    while ((xml.length()!=0)&&(isspace(xml.front())))
        xml.erase(0,1);
    while ((xml.length()!=0)&&(isspace(xml.back())))
        xml.pop_back();
}

bool XMLParser::parse( std::string xml ) {
    this->xml = xml;
    depth = 0;
    bool rc = inner();
    if ( rc ) {
        m_result = onFinish();
    }
    return rc;
}

bool XMLParser::istext( std::string tk ) {
    return tk.front()!='<';
}

void XMLParser::extractAttributes( std::string& r, std::string tk ) {
    r="";
    size_t pos = 0;
    while( (pos!=tk.length()) && ((tk.at(pos)=='<')||(tk.at(pos)=='/')) ) pos++;
    while( (pos!=tk.length()) && (!isspace(tk.at(pos))) ) pos++;
    if ( pos == tk.length() ) return;
    while( (pos!=tk.length()) && (isspace(tk.at(pos))) ) pos++;
    if ( pos == tk.length() ) return;
    r = tk.substr(pos,std::string::npos);
    while( (r.back()=='>')||(r.back()=='/') ) r.pop_back();
    while( (r.length()!=0) && (isspace(r.back())) ) r.pop_back();
}

void XMLParser::extractTag( std::string& r, std::string tk ) {
    size_t pos = 0;

    while( (pos!=tk.length()) && ((tk.at(pos)=='<')||(tk.at(pos)=='/')) ) pos++;

    while( (pos!=tk.length()) && (tk.at(pos)!='/') && (tk.at(pos)!='>') && ( !isspace(tk.at(pos)) ) ) {
        r+=tk.at(pos++);
    }
}

bool XMLParser::isstart( std::string tk ) {
    bool rc = false;
    if ( tk.front() == '<' ) {
        if ( tk.back() == '>' ) {
            if ( tk.at(1) != '/' ) {
                rc = true;
            }
        }
    }
    return rc;
}

bool XMLParser::iswhole( std::string tk ) {
    bool rc = isstart( tk );
    if ( rc ) {
        if ( tk.at( tk.length()-2 ) != '/' )
            rc = false;
    }
    return rc;
}

bool XMLParser::next( std::string& tk ) {
    bool rc = true;
    skipSpace();
    if ( xml.length() !=0 ) {
        if ( xml.front() !=  '<' ) {
            while ((xml.length()!=0) && ( xml.front()!='<' ) ) {
                tk+=xml.front();
                xml.erase(0,1);
            }
            trimSpace(tk);
            if ( xml.length() == 0 ) {
                std::cerr << "unterminated text\n";
                rc = false;
            }
        } else {
            while ((xml.length()!=0) && ( xml.front()!='>' ) ) {
                tk+=xml.front();
                xml.erase(0,1);
            }
            if ( xml.length() == 0 ) {
                std::cerr << "unterminated tag\n";
                rc = false;
            }
            tk+=xml.front();
            xml.erase(0,1);
        }
    }
    return rc;
}

bool XMLParser::nextAttribute( std::string& key, std::string& value, std::string& source ) {
    trimSpace(source);
    if ( source.length() == 0 ) return false;
    key="";
    value="";

    while((source.length()!=0)&&(source.front()!='=')) {
        key+=source.front();
        source.erase(0,1);
    }
    if ( source.length()==0 ) {
        std::cerr << "bad attributes\n";
        return false;
    }
    source.erase(0,1);
    trimSpace(source);

    char c = source.front();
    if ( (c=='\'') || ( c=='\"' ) ) {
        source.erase(0,1);
        for (;;) {
            if ( source.length() == 0 ) {
                std::cerr << "unterminated attribute value\n";
                return false;
            }

            if ( source.front() == '\\' ) {
                value+=source.front();
                source.erase(0,1);
                value+=source.front();
                source.erase(0,1);
            } else if ( source.front() == c ) {
                source.erase(0,1);
                break;
            } else {
                value+=source.front();
                source.erase(0,1);
            }
        }
    } else {
        while ((source.length()!=0)&&(!isspace(source.front()))) {
            value+=source.front();
            source.erase(0,1);
        }
    }

    return true;
}

bool XMLParser::inner() {
    depth++;

    for (;;) {
        skipSpace();
        if ( xml.length() == 0 ) 
            break;

        std::string n;
        if ( !next( n ) ) return false;
    
        if ( istext( n ) ) {
            onText( n );
        } else {
            if ( isstart( n ) ) {
                std::string res;
                extractTag( res, n );
                onStartTag( res );

                extractAttributes( res, n );

                std::string key;
                std::string value;
                while (nextAttribute( key, value, res ) )
                    onAttribute( key, value );

                if ( iswhole( n ) ) {
                    onEndTag( res );
                } else {
                    if ( inner() == false ) return false;
                }
            } else {
                /* if it isn't text or start it must be end */
                std::string res;
                extractTag( res, n );
                onEndTag( res );
                depth--;
                return true;
            }
        }
    }
    depth--;
    return true;
}


