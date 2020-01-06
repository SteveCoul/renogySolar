#ifndef __common_hpp__
#define __common_hpp__

#include <string>
#include <syslog.h>

#include "Args.hpp"

class Common {
public:
    static int shouldQuit( void );
    static unsigned long long NOWms( void );
};

typedef int (*bootstrap)( Args* args );
bootstrap getclass( void );
const char* defaultargs( void );

#define ENTRYPOINT( app ) bootstrap getclass( void ) { return app::main; }
#define DEFAULT_ARGS( text ) const char* defaultargs( void ) { return text; }

#endif

