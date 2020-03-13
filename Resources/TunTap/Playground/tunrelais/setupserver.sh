#!/bin/bash
# Setup the client side.

sudo gcc tunrelais.c -o tunerelais
sudo gcc tcpserver.c -o tcpserver


sudo ip link delete tun66
sleep 1

sudo ./tunrelais -i tun66 -s &

sudo ip link set tun66 up
sudo ip addr add 10.0.2.1/24 dev tun66

sudo ./tcpserver
