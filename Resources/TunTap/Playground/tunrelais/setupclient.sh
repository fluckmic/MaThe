#!/bin/bash
# Setup the client side.

ip link delete tun33 2> /dev/null
ip link delete tun34 2> /dev/null
kill $(pidof tunrelaisclient) 2> /dev/null

sleep 2

gcc tunrelaisclient.c -o tunrelaisclient
gcc tcpclient.c       -o tcpclient

ip link set dev lo              multipath off
ip link set dev enp0s3          multipath off
ip link set dev enp0s8          multipath off
ip link set dev docker0         multipath off
ip link set dev br-f4bf2f554158 multipath off

sleep 1

./tunrelaisclient &

sleep 5

ip link set tun33 up
ip addr add 10.0.1.1/24 dev tun33

ip link set tun34 up
ip addr add 10.0.3.1/24 dev tun34

sleep 1

./tcpclient
