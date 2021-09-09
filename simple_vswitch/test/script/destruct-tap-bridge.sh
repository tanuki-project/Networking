#!/bin/sh

../../bin/cmd/vswconfig delete veth1-vsw > /dev/null
../../bin/cmd/vswconfig delete veth2-vsw > /dev/null
../../bin/cmd/vswconfig delete veth3-vsw > /dev/null
../../bin/cmd/vswconfig delete veth4-vsw > /dev/null

ip netns delete NS1
ip netns delete NS2
ip netns delete NS3
ip netns delete NS4

exit 0

