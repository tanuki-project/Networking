#!/bin/sh

lsmod | grep vswitch > /dev/null
if [ $? -eq 0 ]
then
        echo vswitch is already installed.
        exit 1
fi

insmod ../../bin/drv/vswitch.ko
if [ $? -ne 0 ]
then
        echo failed to install vswitch.
        exit 1
fi

MAJOR=`awk "\\$2==\"vswitch\" {print \\$1}" /proc/devices 2>/dev/null`
mknod /dev/vswitch c $MAJOR 0
if [ $? -ne 0 ]
then
        echo failed to create /dev/vswitch.
        exit 1
fi

echo vswitch is installed.
exit 0
