#!/bin/sh
FNAME=/tmp/checkHHB.FAILURE.txt

pgrep homeheart
if [ $? -ne 0 ] ; then
   date >> $FNAME
   echo " --- HomeheartBeat has stopped, restarting " >> $FNAME
   cd /home/pconroy/HomeHeartBeat 
   ./dist/Debug/GNU-Linux/homeheartbeat_1 &
fi

