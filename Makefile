
COPTS=-pedantic -Wall -Werror

all:	rs485-tcp controller_read webserver history

webserver: webserver.cpp Common.cpp
	cc $(COPTS) -o $@ $^
clean::
	rm -f webserver
	
controller_read: controller_read.cpp Common.cpp modbus.cpp
	cc $(COPTS) -o $@ $^
clean::
	rm -f controller_read

rs485-tcp: rs485-tcp.cpp Common.cpp
	cc $(COPTS) -o $@ $^
clean::
	rm -f rs485-tcp

history: history.cpp Common.cpp modbus.cpp HistoryTable.cpp
	c++ $(COPTS) -o $@ $^ -lsqlite3 

clean::
	rm -f history

doc: documentation/html/index.html

documentation/html/index.html: *.cpp *.hpp Doxyfile
	doxygen

clean::
	rm -rf documentation
