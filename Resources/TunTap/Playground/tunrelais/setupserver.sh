#!/bin/bash
# Setup the client side.

ip link delete tun66 2> \dev\null
ssleep 1

gcc tunrelais.c -o tunerelais
gcc tcpserver.c -o tcpserver

./tunrelais -i tun66 -s &
sleep 1

ip link set tun66 up
ip addr add 10.0.2.1/24 dev tun66

./tcpserver
