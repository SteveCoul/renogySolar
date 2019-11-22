#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include <iostream>
#include <string>

#include "Args.hpp"
#include "Common.hpp"

#define PLACEHOLDER_FOR_BOOLEAN "placeholder_value"

static
int stricmp( const char* s1, const char* s2 ) {
    while ( ( s1[0] != '\0') && ( s2[0] != '\0' ) ) {
        char c1 = *s1++;
        char c2 = *s2++;
        if ( ( c1 >= 'a' ) && ( c1 <= 'z' ) ) c1-=32;
        if ( ( c2 >= 'a' ) && ( c2 <= 'z' ) ) c2-=32;           
        if ( c1 != c2 ) return c1-c2;
    }
    return 0;
}

Args::Args() {
}

Args::~Args() {
}

int Args::process( int argc, char** argv, const char* default_args ) {

    m_option.clear();
    m_value.clear();
    m_default.clear();
    m_help.clear();
    m_list.clear();

    parseDefaults( "-single:Run task as a single process rather than using the bootstrap code:#" );
    parseDefaults( default_args );

    // check for help
    for ( int i = 1; i < argc; i++ ) {
        if ( ( stricmp( argv[i], "-h" ) == 0 ) ||
             ( stricmp( argv[i], "-?" ) == 0 ) ||
             ( stricmp( argv[i], "-help" ) == 0 ) ) {
                showHelp( argv[0] );
                return -1;
        }
    }

    return parseArgs( argc, argv );
}

const char* Args::getOptionAsString( const char* what ) {
    int i = findOption( what );
    if ( i < 0 ) {
        log( LOG_ERR, "failed to find command line option value for '%s'", what );
        return "";
    }
    return m_value[i].c_str();
}

int Args::getOptionAsInt( const char* what ) {
    return (int)atoi( getOptionAsString( what ) );
}

unsigned int Args::getOptionAsUnsignedInt( const char* what ) {
    return (unsigned int)getOptionAsInt( what );
}

int Args::countList( void ) {
    return (int)m_list.size();
}

const char* Args::getListItem( unsigned int idx ) {
    return m_list[idx].c_str();
}

int Args::getListItemAsInt( unsigned int idx ) {
    return atoi( getListItem( idx ) );
}

bool Args::getOptionAsBoolean( const char* what ) {
    const char* i = getOptionAsString( what );
    bool rc = false;
    if ( i ) {
        if ( stricmp( i, "true" ) == 0 ) rc = true;
        else if ( stricmp( i, "on" ) == 0 ) rc = true;
        else if ( atoi( i ) != 0 ) rc = true;
    }
    return rc;  
}

void Args::showHelp( const char* appname ) {
    std::cerr << "\n";
    std::cerr << appname << " <args>";
    if ( m_listHelp.length() ) {
        std::cerr << " <" << m_listHelp << ">";
    }
    std::cerr << "\n";
    for ( size_t i = 0; i < m_option.size(); i++ ) {
        if ( m_default.at(i) == PLACEHOLDER_FOR_BOOLEAN )
            std::cerr << "\t-" << m_option.at(i) << "\t\t" << m_help.at(i) << "\n";
        else
            std::cerr << "\t-" << m_option.at(i) << "\t\t" << m_help.at(i) << ", default " << m_default.at(i) << "\n";
    }
    std::cerr << "\n";
}

void Args::parseDefaults( const char* default_args ) {
    char* save_ptr = NULL;
    const char* delims = "-";
    char* df = strdup( default_args );
    const char* s = strtok_r( df, delims, &save_ptr );
    while ( s != NULL ) {
        if ( s[0] == ':' ) {
            m_listHelp = std::string(s+1);
            break;
        }

        char* save_ptr2 = NULL;
        const char* delims2 = ":";

        char* s2 = strdup( s );

        const char* n = strtok_r( s2, delims2, &save_ptr2 );
        const char* t = strtok_r( NULL, delims2, &save_ptr2 );
        const char* d = strtok_r( NULL, delims2, &save_ptr2 );

        /* trusted client :-) no error checking */
        m_option.push_back( std::string( n ) );
        m_help.push_back( std::string( t ) );
        if ( d[0] == '#' ) {
            m_default.push_back( std::string( PLACEHOLDER_FOR_BOOLEAN ) );
            m_value.push_back( std::string( "0" ) );
        } else {
            m_default.push_back( std::string( d ) );
            m_value.push_back( std::string( d ) );
        }
        s = strtok_r( NULL, delims, &save_ptr );

        free( s2 );
    }
    free( df );
}

int Args::parseArgs( int argc, char** argv ) {
    int i = 1;
    while ( i < argc ) {
        if ( argv[i][0] != '-' ) {
            while ( i < argc ) {
                m_list.push_back( std::string( argv[i] ) );
                i++;
            }
            break;
        }

        int o = findOption( argv[i]+1 );
        if ( o < 0 ) {
            std::cerr << "illegal command option '" << argv[i] << "'\n";
            showHelp( argv[0] );
            return -1;
        }

        if ( m_default[o] == PLACEHOLDER_FOR_BOOLEAN ) {
            m_value[o] = std::string( "1" );
            i+=1;
        } else {
            m_value[o] = std::string( argv[i+1] );
            i+=2;
        }
    }

    return 0;
}

int Args::findOption( const char* name ) {
    for ( unsigned int i = 0; i < m_option.size(); i++ ) {
        if ( m_option.at(i) == name ) 
            return (int)i;
    }
    return -1;
}

