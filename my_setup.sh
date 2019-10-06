#!/bin/sh

# I have a battery bank run by two renogy CMD-40 charge controllers, each with it's own array.

# The two controllers are addressed as unit 1 and unit 2 ( programmed using MT50 interface ).

# This script is run on boot on a raspberry PI to which a RS485 shifter is attached to UART0

# The communication channels of both controllers are wired in parallel to the PI.


# Expose UART ( on currently hardcoded TCP port 32700 )
./rs485-tcp /dev/ttyUSB0 &

# Install controller access service for first controller to port 32701 using RS485-TCP on port 37000
./controller_read 127.0.0.1 32700 1 32701&

# Install controller access service for second controller to port 32702 using RS485-TCP on port 37000
./controller_read 127.0.0.1 32700 2 32702&

# Run a database logging task usinged RS485-TCP on port 37000 for both controllers and expose on port 32900
./history 127.0.0.1 32700 32900 history.db 1 2&

# Serve the HTML/JS UI on port 80
./webserver 80

