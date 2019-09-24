#!/bin/sh
./rs485-tcp /dev/ttyUSB0 &
./controller_read 127.0.0.1 32700 1 32701&
./controller_read 127.0.0.1 32700 2 32702&
./history 127.0.0.1 32700 32900 history.db 1 2&
./webserver 80

