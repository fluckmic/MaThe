#!/bin/bash
# Setup the client side.

ip link delete tun33 2> /dev/null
ip link delete tun34 2> /dev/null
kill $(pidof tunrelais) 2> /dev/null
kill $(pidof tunclient) 2> /dev/null

sleep 1

gcc tunrelais.c -o tunrelais
gcc tunclient.c -o tunclient
gcc tcpclient.c -o tcpclient

ip link set dev lo              multipath off
ip link set dev enp0s3          multipath off
ip link set dev enp0s8          multipath off
ip link set dev docker0         multipath off
ip link set dev br-f4bf2f554158 multipath off

./tunclient &
sleep 1
./tunrelais -i tun33 -c 192.168.1.45 &

ip link set tun33 up 2> /dev/null
ip addr add 10.0.1.1/24 dev tun33 2> /dev/null

ip link set tun34 up 2> /dev/null
ip addr add 10.0.3.1/24 dev tun34 2> /dev/null

sleep 1

./tcpclient
