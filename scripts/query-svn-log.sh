#!/bin/bash

usage()
{
    echo "Usage: $0 <path> <last_rev> <delay>"
}

if [ $# -lt 3 ]; then
    usage
    exit 1
fi

SVNPATH="$1"
LASTREV="$2"
DELAY="$3"

while [ 1 -gt 0 ];
do
    #echo "Query svn log: -r$LASTREV:HEAD $SVNPATH"

    svn log -r$LASTREV:HEAD "$SVNPATH" > .tmp 2>/dev/null

    rn=`grep ^r[1-9] .tmp | tail -1 | awk '{print $1}'`
    if [ "x$rn" != "x" ] ;then
        n=${rn##r}
        if [ $n -gt 0 ]; then
            let LASTREV=n+1
        fi
        cat .tmp
    fi

    :> .tmp

    sleep $DELAY
done

