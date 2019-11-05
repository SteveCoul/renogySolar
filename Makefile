
COPTS=-g -pedantic -Wall -Werror -std=c++11

all:	rs485-tcp controller_read webserver history

webserver: src/webserver.cpp src/Common.cpp src/Args.cpp
	c++ $(COPTS) -o $@ $^
clean::
	rm -f webserver

controller_read: src/controller_read.cpp src/Common.cpp src/ModBus.cpp src/Args.cpp
	c++ $(COPTS) -o $@ $^
clean::
	rm -f controller_read

rs485-tcp: src/rs485-tcp.cpp src/Common.cpp src/Args.cpp src/SerialPort.cpp
	c++ $(COPTS) -o $@ $^
clean::
	rm -f rs485-tcp

history: src/history.cpp src/Common.cpp src/ModBus.cpp src/HistoryTable.cpp src/Args.cpp
	c++ $(COPTS) -o $@ $^ -lsqlite3 

clean::
	rm -f history

doc: documentation/html/index.html

documentation/html/index.html: src/*.cpp src/*.hpp Doxyfile
	doxygen

clean::
	rm -rf documentation
