#!/bin/bash
# Setup the client side.

sudo ip link delete tun33
sleep 1

sudo ./tunrelais -i tun33 -c 127.0.0.1 &

sudo ip link set tun33 up
sudo ip addr add 10.0.1.1/24 dev tun33

sudo ./tcpclient
