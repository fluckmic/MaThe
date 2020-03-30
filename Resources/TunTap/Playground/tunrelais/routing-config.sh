#!/bin/bash

# Assumes that the virtual tun interfaces are already alocated.

# Create namespaces
ip netns add shila-ingress
ip netns add shila-engress

# Assign interfaces
ip link set tun33 netns shila-engress
ip link set tun34 netns shila-engress
ip link set tun35 netns shila-engress

ip link set tun66 netns shila-ingress

# Brings the interfaces up and assigns the subnets
ip netns exec shila-engress ip addr add 10.0.1.1/24 dev tun33
ip netns exec shila-engress ip link set tun33 up

ip netns exec shila-engress ip addr add 10.0.2.1/24 dev tun34
ip netns exec shila-engress ip link set tun34 up

ip netns exec shila-engress ip addr add 10.0.3.1/24 dev tun35
ip netns exec shila-engress ip link set tun35 up

ip netns exec shila-ingress ip addr add 10.7.0.9/24 dev tun66
ip netns exec shila-ingress ip link set tun66 up

# Creates the routing entries necessary for MPTCP
ip netns exec shila-engress ip rule add from 10.0.1.1 table 1
ip netns exec shila-engress ip route add table 1 default dev tun33 scope link

ip netns exec shila-engress ip rule add from 10.0.2.1 table 2
ip netns exec shila-engress ip route add table 2 default dev tun34 scope link

ip netns exec shila-engress ip rule add from 10.0.3.1 table 3
ip netns exec shila-engress ip route add table 3 default dev tun35 scope link

ip netns exec shila-engress ip rule add to 10.7.0.9 iif lo table 1

ip netns exec shila-ingress ip rule add from 10.7.0.9 table 1
ip netns exec shila-ingress ip route add table 1 default dev tun66 scope link

ip netns exec shila-ingress ip link set dev lo multipath off
ip netns exec shila-engress ip link set dev lo multipath off
