# renogySolar
Tools for monitoring a Solar power system based on Renogy Commander MPPT charge controllers

## RS485-TCP
Simple application to expose a UART ( to which we have an RS485 level shifter attached ) 
over a TCP port. The idea being that other services can open the port, sen and receive
from the UART, then close the port. 

Rather than having multiple services fighting over the port, especially since it's a half
duplex system.

## controller_read
Uses the RS485-TCP service to read the renogy MPPT charge controller using MODBUS protocol.
Exposes a TCP server port which acts like a dumb HTTP server. Any request made will result
in the status of the controller being passed back as an XML file wrapped in a HTTP response.

## history
Logs data for all specified controllers. Uses MODBUS protocol over RS485-TCP in order to 
keep a log of voltate and current parameters in a SQLite database. Keeps records every
second for the last minute, every minute for the last hour and every hour for the last day
etc. These databases can be queried in XML format over a TCP server port which acts as
a simple HTTP server.

## webserver
Used to serve the HTML/Javascript UI pages

## Client facing webpages
Provide the actual application UI. A dynamic website that reflects the status of the 
solar farm.

