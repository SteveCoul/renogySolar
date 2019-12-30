FROM ubuntu:16.04
RUN apt-get -y update
RUN adduser renogysolar
WORKDIR /home/renogysolar
RUN apt-get -y install bash
RUN rm -f /bin/sh && ln -s /bin/bash /bin/sh
RUN apt-get -y install net-tools
RUN apt-get -y install make
RUN apt-get -y install autoconf
RUN apt-get -y install automake
RUN apt-get -y install libtool
RUN apt-get -y install gcc
RUN apt-get -y install g++
RUN apt-get -y install libsqlite3-dev
RUN apt-get -y install doxygen graphviz
USER renogysolar
WORKDIR /home/renogysolar


