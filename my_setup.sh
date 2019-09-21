#!/bin/sh
rs485-tcp /dev/ttyUSB0
controller_read 127.0.0.1 32700 1 &
controller_read 127.0.0.1 32700 2
