
all:	rs485-tcp controller_read webserver

webserver: webserver.cpp
	cc -o $@ $^
clean::
	rm -f webserver.cpp
	
controller_read: controller_read.cpp
	cc -o $@ $^
clean::
	rm -f controller_read

rs485-tcp: rs485-tcp.cpp
	cc -o $@ $^
clean::
	rm -f rs485-tcp

