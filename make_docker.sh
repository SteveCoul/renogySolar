#!/bin/sh

PROJECT_NAME=renogysolar

# Kill all existing containers for the project
EXIST_CONTAINERS=`docker ps -a --filter "ancestor=$PROJECT_NAME" -q`
docker kill $EXIST_CONTAINERS 2>/dev/null
docker rm $EXIST_CONTAINERS 2>/dev/null

# Remove all existing images. 
docker rmi $PROJECT_NAME 2>/dev/null

# Build container
docker build -t $PROJECT_NAME --no-cache .

# How to use
# docker run -v $PWD:/home/renogysolar renogysolar make
