#!/bin/bash
# Setup the client side.

ip link delete tun33 2> /dev/null
ip link delete tun34 2> /dev/null
kill $(pidof tunrelaisclient) 2> /dev/null

sysctl net.mptcp.mptcp_scheduler=redundant

sleep 2

gcc tunrelaisclient.c -o tunrelaisclient -g
gcc tcpclient.c       -o tcpclient -g

ip link set dev lo              multipath off
ip link set dev ens33           multipath off
ip link set dev br-26cedfcb1b6d multipath off
ip link set dev docker0         multipath off
ip link set dev ens34           multipath off

sleep 1

./tunrelaisclient &

sleep 5

ip link set tun33 up
ip addr add 10.0.1.1/24 dev tun33

ip link set tun34 up
ip addr add 10.0.3.1/24 dev tun34

sleep 1

./tcpclient
