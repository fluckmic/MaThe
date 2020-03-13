#!/bin/bash
# Setup the client side.

ip link delete tun33 2> /dev/null
kill $(pidof tunrelais) 2> /dev/null
sleep 1

gcc tunrelais.c -o tunerelais
gcc tcpclient.c -o tcpclient

./tunrelais -i tun33 -c 192.168.1.45 -d &

ip link set tun33 up 2> /dev/null
ip addr add 10.0.1.1/24 dev tun33 2> /dev/null

./tcpclient
