#ifndef __Args_hpp__
#define __Args_hpp__

#include <string>
#include <vector>

///
/// \class Args
/// \brief	Class to process commandline arguments.
///
class Args {
public:
	Args();
	~Args();
	int process( int argc, char** argv, const char* default_args );
	const char* getOptionAsString( const char* what );
	int getOptionAsInt( const char* what );
	unsigned int getOptionAsUnsignedInt( const char* what );
	bool getOptionAsBoolean( const char* what );
	int countList( void );
	const char* getListItem( unsigned int idx );
	int getListItemAsInt( unsigned int idx );
private:
	void showHelp( const char* appname );
	void parseDefaults( const char* default_args );
	int parseArgs( int argc, char** argv );
	int findOption( const char* name );
private:
	std::vector<std::string>		m_option;
	std::vector<std::string>		m_value;
	std::vector<std::string>		m_default;
	std::vector<std::string>		m_help;
	std::vector<std::string>		m_list;
	std::string						m_listHelp;
};

#endif

