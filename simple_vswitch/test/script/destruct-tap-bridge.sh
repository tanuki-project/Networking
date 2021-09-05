#!/bin/sh

../../bin/cmd/vswconfig delete veth0-in > /dev/null
../../bin/cmd/vswconfig delete veth1-in > /dev/null
../../bin/cmd/vswconfig delete veth2-in > /dev/null
../../bin/cmd/vswconfig delete veth3-in > /dev/null

ip netns delete veth0
ip netns delete veth1
ip netns delete veth2
ip netns delete veth3

exit 0

