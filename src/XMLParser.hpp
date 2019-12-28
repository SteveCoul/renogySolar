#ifndef __XMLParser_hpp__
#define __XMLParser_hpp__

#include <string>

class XMLParser {
public:
    virtual void onStartTag( std::string name ) { }
    virtual void onEndTag( std::string name )   { }
    virtual void onAttribute( std::string name, std::string value ) { }
    virtual void onText( std::string text )   { }
    virtual std::string onFinish() { return "ok"; }

    bool parse( std::string xml );
    std::string result() { return m_result; }
private:
    bool isspace( char c );
    bool istext( std::string tk );
    bool isstart( std::string tk );
    bool iswhole( std::string tk );
    bool inner();
    bool next( std::string& tk );
    void skipSpace();
    void trimSpace( std::string& xml );
    void extractTag( std::string& r, std::string tk );
    void extractAttributes( std::string& r, std::string tk );
    bool nextAttribute( std::string& key, std::string& value, std::string& source );
private:
    std::string xml;
    int depth;
    std::string m_result;
};

#endif


