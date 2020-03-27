#!/bin/bash
# Setup the server side.

ip link delete tun66 2> /dev/null

kill $(pidof tunrelais) 2> /dev/null
killall wireshark

sleep 1

gcc tunrelais.c -o tunrelais
gcc tcpserver.c -o tcpserver

./tunrelais -i tun66 -s &
sleep 1

ip link set tun66 up
ip addr add 10.7.0.9/24 dev tun66

wireshark &

sleep 1

./tcpserver
