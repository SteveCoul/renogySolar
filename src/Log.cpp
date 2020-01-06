
#include <stdlib.h>
#include <syslog.h>

#include "Log.hpp"

static int LOG_ALERT_enabled = 0;
static int LOG_CRIT_enabled = 0;
static int LOG_ERR_enabled = 0;
static int LOG_WARNING_enabled = 0;
static int LOG_NOTICE_enabled = 0;
static int LOG_INFO_enabled = 0;
static int LOG_DEBUG_enabled = 0;

void Log::configureLogging( void ) {
    const char* e = getenv( "RENOGY_DEBUG" );
    if ( e == NULL ) e = "EWN";

    openlog( NULL, LOG_PERROR, LOG_USER );

    /* These are always on */
    LOG_ALERT_enabled = 1;  
    LOG_CRIT_enabled = 1;

    /* These maybe */
    for ( int i = 0; e[i] != '\0'; i++ ) {
        switch( e[i] ) {
        case 'e':   
        case 'E':
            LOG_ERR_enabled = 1;
            break;
        case 'w':   
        case 'W':
            LOG_WARNING_enabled = 1;
            break;
        case 'n':   
        case 'N':
            LOG_NOTICE_enabled = 1;
            break;
        case 'i':   
        case 'I':
            LOG_INFO_enabled = 1;
            break;
        case 'd':   
        case 'D':
            LOG_DEBUG_enabled = 1;
            break;
        case '*':
            LOG_ERR_enabled = 1;
            LOG_WARNING_enabled = 1;
            LOG_NOTICE_enabled = 1;
            LOG_INFO_enabled = 1;
            LOG_DEBUG_enabled = 1;
            break;
        default:
            /* ignore unknown values */
            break;
        }
    }

    syslog( LOG_INFO, "Logging Configured Alert=%s, Critical=%s, Error=%s, Warning=%s, Notice=%s, Info=%s, Debug=%s",
                        LOG_ALERT_enabled ? "on" : "off",
                        LOG_CRIT_enabled ? "on" : "off",
                        LOG_ERR_enabled ? "on" : "off",
                        LOG_WARNING_enabled ? "on" : "off",
                        LOG_NOTICE_enabled ? "on" : "off",
                        LOG_INFO_enabled ? "on" : "off",
                        LOG_DEBUG_enabled ? "on" : "off" );
}

void Log::log( int level, const char* fmt, ... ) {
    int output = 0;
    const char* output_prefix;

    switch( level ) {
    case LOG_ALERT:
        output = LOG_ALERT_enabled;
        output_prefix="A";
        break;
    case LOG_CRIT:
        output = LOG_CRIT_enabled;
        output_prefix="C";
        break;
    case LOG_ERR:
        output = LOG_ERR_enabled;
        output_prefix="E";
        break;
    case LOG_WARNING:
        output = LOG_WARNING_enabled;
        output_prefix="W";
        break;
    case LOG_NOTICE:
        output = LOG_NOTICE_enabled;
        output_prefix="N";
        break;
    case LOG_INFO:
        output = LOG_INFO_enabled;
        output_prefix="I";
        break;
    case LOG_DEBUG:
        output = LOG_DEBUG_enabled;
        output_prefix="D";
        break;
    default:
        output = 0;
        break;
    }

    if ( output ) {
        std::string new_fmt = std::string("[");
        new_fmt += output_prefix;
        new_fmt += "] ";
        new_fmt += fmt;
        va_list args;
        va_start( args, fmt );
        vsyslog( level, new_fmt.c_str(), args );
        va_end( args );
    }
}

void Log::log( int level, std::string str ) {
	log( level, str.c_str() );
}


