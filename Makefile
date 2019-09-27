
all:	rs485-tcp controller_read webserver history

webserver: webserver.cpp common.cpp
	cc -o $@ $^
clean::
	rm -f webserver.cpp
	
controller_read: controller_read.cpp common.cpp modbus.cpp
	cc -o $@ $^
clean::
	rm -f controller_read

rs485-tcp: rs485-tcp.cpp common.cpp
	cc -o $@ $^
clean::
	rm -f rs485-tcp

history: history.cpp common.cpp modbus.cpp
	c++ -o $@ $^ -lsqlite3 

clean::
	rm -f history

