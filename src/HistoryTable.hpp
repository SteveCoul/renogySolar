
#ifndef __HistoryHistoryTable_hpp__
#define __HistoryHistoryTable_hpp__

#include <string>

///
/// \class HistoryTable
///
/// \brief  Implements a table in the history database.
///
/// Voltage and current records can be added to a table, and the table be read back as XML.
/// Table automatically discards records outside of a specified time window, table also can
/// automatically forward the average record values for a given time window on to another 
/// table. ( which means the last 60 seconds can be passed on to the 'minutes' table etc. )
///
class HistoryTable;

typedef void (HistoryTable::*callback_fn)( void* param, const char* time, float voltage, float current );

class HistoryTable {
public:
    HistoryTable( const char* name );
    ~HistoryTable();
    void addRecord( time_t now, int id, float input_voltage, float input_current );
    void setWindow( time_t t );
    void setExpiry( time_t t );
    void cascade( time_t time_mod, class HistoryTable* to );
    std::string toXML( int id );
private:
    void doAveraging( void* param, const char* t, float voltage, float current );
    void toXMLcallback( void* param, const char* t, float voltage, float current );
    void forEachInTimeRange( int id, const char* start_time, const char* end_time, callback_fn callback, void* param );
private:
    struct averaging {
        int count;
        float voltage;  
        float current;
    };
private:
    const char*     m_name;
    time_t          m_window;
    time_t          m_expiry;
    time_t          m_cascade_time;
    class HistoryTable* m_cascade_target;
    class HistoryTable* m_next;
    sqlite3_stmt*   m_for_each_in_range;
public:
    static HistoryTable* get( const char* name );
    static void mktimestamp( char* timestamp, time_t now );
    static void sqlExec( const char* command );
    static int cinit( const char* database );
private:
    static  sqlite3*        c_db;
    static class HistoryTable*      c_list;
};

#endif

