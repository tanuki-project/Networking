#!/bin/sh

ip link add veth1-vsw type veth peer name veth1
ip link add veth2-vsw type veth peer name veth2
ip link add veth3-vsw type veth peer name veth3
ip link add veth4-vsw type veth peer name veth4

ip netns add NS1
ip netns add NS2
ip netns add NS3
ip netns add NS4

ip link set veth1 netns NS1
ip link set veth2 netns NS2
ip link set veth3 netns NS3
ip link set veth4 netns NS4

ip link set veth1-vsw up
ip netns exec NS1 ip link set veth1 up
ip netns exec NS1 ip address add 192.168.100.101/24 dev veth1
ip netns exec NS1 ip link set lo up
ip netns exec NS1 ip address add 127.0.0.1/24 dev lo

ip link set veth2-vsw up
ip netns exec NS2 ip link set veth2 up
ip netns exec NS2 ip address add 192.168.100.102/24 dev veth2
ip netns exec NS2 ip link set lo up
ip netns exec NS2 ip address add 127.0.0.1/24 dev lo

ip link set veth3-vsw up
ip netns exec NS3 ip link set veth3 up
ip netns exec NS3 ip address add 192.168.100.103/24 dev veth3
ip netns exec NS3 ip link set lo up
ip netns exec NS3 ip address add 127.0.0.1/24 dev lo

ip link set veth4-vsw up
ip netns exec NS4 ip link set veth4 up
ip netns exec NS4 ip address add 192.168.100.104/24 dev veth4
ip netns exec NS4 ip link set lo up
ip netns exec NS4 ip address add 127.0.0.1/24 dev lo

ip addr show veth1-vsw
ip netns exec NS1 ip addr show veth1
ip addr show veth2-vsw
ip netns exec NS2 ip addr show veth2
ip addr show veth3-vsw
ip netns exec NS3 ip addr show veth3
ip addr show veth4-vsw
ip netns exec NS4 ip addr show veth4

../../bin/cmd/vswconfig add veth1-vsw > /dev/null
../../bin/cmd/vswconfig add veth2-vsw > /dev/null
../../bin/cmd/vswconfig add veth3-vsw > /dev/null
../../bin/cmd/vswconfig add veth4-vsw > /dev/null

exit 0

