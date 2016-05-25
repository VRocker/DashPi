#!/bin/sh
#
# UPS Pico shutdown script
#

# Send a SIGTERM to the camera recorder
kill `pidof recorder.bin` &
echo "Waiting for recorder to exit..."
while true ; do
        PID=`ps cat | grep recorder.bin | grep -v grep`
        if [ -z "$PID" ]; then
		echo "Killed!"
		break
	fi
	sleep 1
done

# Power off the system
poweroff