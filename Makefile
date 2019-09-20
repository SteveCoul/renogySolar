
all:	rs485-tcp

rs485-tcp: rs485-tcp.cpp
	cc -o $@ $^
clean::
	rm -f rs485-tcp

