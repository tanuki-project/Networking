#!/bin/sh

lsmod | grep vswitch > /dev/null
if [ $? -ne 0 ]
then
        echo vswitch in not installed.
        exit 0
fi

rmmod vswitch > /dev/null
if [ $? -ne 0 ]
then
        echo failed to uninstall vswitch.
        exit 1
fi

rm /dev/vswitch
echo vswitch is uninstalled.

exit 0

