#!/bin/sh

ip link add veth0-in type veth peer name veth0-out
ip link add veth1-in type veth peer name veth1-out
ip link add veth2-in type veth peer name veth2-out
ip link add veth3-in type veth peer name veth3-out

ip netns add veth0
ip netns add veth1
ip netns add veth2
ip netns add veth3

ip link set veth0-out netns veth0
ip link set veth1-out netns veth1
ip link set veth2-out netns veth2
ip link set veth3-out netns veth3

ip link set veth0-in up
ip netns exec veth0 ip link set veth0-out up
ip netns exec veth0 ip address add 192.168.100.101/24 dev veth0-out

ip link set veth1-in up
ip netns exec veth1 ip link set veth1-out up
ip netns exec veth1 ip address add 192.168.100.102/24 dev veth1-out

ip link set veth2-in up
ip netns exec veth2 ip link set veth2-out up
ip netns exec veth2 ip address add 192.168.100.103/24 dev veth2-out

ip link set veth3-in up
ip netns exec veth3 ip link set veth3-out up
ip netns exec veth3 ip address add 192.168.100.104/24 dev veth3-out

ip addr show veth0-in
ip netns exec veth0 ip addr show veth0-out
ip addr show veth1-in
ip netns exec veth1 ip addr show veth1-out
ip addr show veth2-in
ip netns exec veth2 ip addr show veth2-out
ip addr show veth3-in
ip netns exec veth3 ip addr show veth3-out

../../bin/cmd/vswconfig add veth0-in > /dev/null
../../bin/cmd/vswconfig add veth1-in > /dev/null
../../bin/cmd/vswconfig add veth2-in > /dev/null
../../bin/cmd/vswconfig add veth3-in > /dev/null

exit 0

