
COPTS=-pedantic -Wall -Werror -std=c++11

all:	rs485-tcp controller_read webserver history test

webserver: src/webserver.cpp src/Common.cpp
	cc $(COPTS) -o $@ $^
clean::
	rm -f webserver

test: src/test.cpp src/Common.cpp src/modbus.cpp
	cc $(COPTS) -o $@ $^
clean::
	rm -f test
	
controller_read: src/controller_read.cpp src/Common.cpp src/modbus.cpp
	cc $(COPTS) -o $@ $^
clean::
	rm -f controller_read

rs485-tcp: src/rs485-tcp.cpp src/Common.cpp
	cc $(COPTS) -o $@ $^
clean::
	rm -f rs485-tcp

history: src/history.cpp src/Common.cpp src/modbus.cpp src/HistoryTable.cpp
	c++ $(COPTS) -o $@ $^ -lsqlite3 

clean::
	rm -f history

doc: documentation/html/index.html

documentation/html/index.html: src/*.cpp src/*.hpp Doxyfile
	doxygen

clean::
	rm -rf documentation
