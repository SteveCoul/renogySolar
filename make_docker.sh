#!/bin/sh

if [ $# -eq 0 ]; then
	$0 renogysolar
	exit 0
fi

PROJECT_NAME=$1
ERASE=""
WHO=`whoami`

# Kill all existing containers for the project
EXIST_CONTAINERS=`docker ps -a --filter "ancestor=$PROJECT_NAME" -q`
docker kill $EXIST_CONTAINERS 2>/dev/null
docker rm $EXIST_CONTAINERS 2>/dev/null

# Remove all existing images. 
docker rmi $PROJECT_NAME 2>/dev/null

# Copy users ssh keys to put inside container
cp ~/.ssh/id_rsa .
cp ~/.ssh/id_rsa.pub .
ERASE+=" id_rsa id_rsa.pub"

# Create sudoers entry
cat <<_EOF_ > $WHO.sudo
$WHO ALL=(ALL) NOPASSWD:ALL
_EOF_
ERASE+=" $WHO.sudo"

# Create the dockerfile
cat <<_EOF_ > Dockerfile
FROM ubuntu:16.04
RUN apt-get -y update
RUN adduser $WHO
WORKDIR /home/$WHO
RUN apt-get -y install ssh
RUN mkdir -p .ssh
ADD id_rsa .ssh/id_rsa
ADD id_rsa.pub .ssh/id_rsa.pub
RUN chmod 600 .ssh/id_rsa
RUN chmod 600 .ssh/id_rsa.pub
RUN chown -R $WHO .ssh
RUN apt-get -y install sudo
ADD $WHO.sudo /etc/sudoers.d/$WHO
RUN apt-get -y install git
RUN apt-get -y install bash
RUN rm -f /bin/sh && ln -s /bin/bash /bin/sh
RUN apt-get -y install vim
RUN apt-get -y install net-tools
RUN apt-get -y install make
RUN apt-get -y install gcc
RUN apt-get -y install g++
RUN apt-get -y install libsqlite3-dev
RUN apt-get -y install doxygen graphviz
USER $WHO
WORKDIR /home/$WHO
_EOF_
ERASE+=" Dockerfile"

# Build container
docker build -t $PROJECT_NAME .

rm -f $ERASE
