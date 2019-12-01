#!/bin/bash

PIDS=""

trap ctrl-c INT

ctrl-c() {
    # todo, wait a bit then forcibly kill any who didn't go
    #    echo "kill $PIDS"
    echo ""
}

runapp() {
    DEBUG=$1
    NAME=$2
    shift
    shift
    ARGS="$@"

    if [ "$DEBUG" == "y" ]; then
        echo "debug: $NAME $ARGS"
        RENOGY_DEBUG="*" valgrind --show-reachable=yes --leak-check=full --track-origins=yes ./$NAME -single $ARGS 2>$NAME.debug.log &
    else
        echo "     : $NAME $ARGS"
        ./$NAME -single $ARGS 2>/dev/null &
    fi

    DONE=$!
    PIDS="$PIDS $DONE"
}

rm -rf *.debug.log
runapp n rs485-tcp -d /dev/ttyUSB0 -p 32700 -b 115200 -w 750
runapp n controller_read -ra 127.0.0.1 -rp 32700 -i 1 -p 32701
runapp n controller_read -ra 127.0.0.1 -rp 32700 -i 2 -p 32702
runapp y history -ra 127.0.0.1 -rp 32700 -p 32900 -f history.db 1 2
runapp n webserver -p 8080
wait
