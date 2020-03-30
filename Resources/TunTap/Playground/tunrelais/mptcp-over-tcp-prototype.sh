#!/bin/bash

# Delete and close all the leftovers from the last run.
ip link delete tun33 2> /dev/null
ip link delete tun34 2> /dev/null
ip link delete tun35 2> /dev/null
ip link delete tun66 2> /dev/null

kill $(pidof mptcp-over-tcp-prototype)  2> /dev/null
kill $(pidof tcpclient)                 2> /dev/null
kill $(pidof tcpserver)                 2> /dev/null
killall wireshark                       2> /dev/null

ip rule delete to 10.7.0.9 iif lo              table tun33

# Load round robin mptcp module and activate it
/sbin/modprobe mptcp_rr
sysctl -w net.mptcp.mptcp_scheduler=roundrobin 2> /dev/null

sleep 2

gcc mptcp-over-tcp-prototype.c -o mptcp-over-tcp-prototype -g
gcc tcpclient.c                -o tcpclient -g
gcc tcpserver.c                -o tcpserver -g

ip link set dev lo              multipath off
ip link set dev ens33           multipath off
ip link set dev br-26cedfcb1b6d multipath off
ip link set dev docker0         multipath off
ip link set dev ens34           multipath off

sleep 1

./mptcp-over-tcp-prototype &

sleep 1

ip link set tun33 up
ip addr add 10.0.1.1/24 dev tun33

ip link set tun34 up
ip addr add 10.0.2.1/24 dev tun34

ip link set tun35 up
ip addr add 10.0.3.1/24 dev tun35

ip link set tun66 up
ip addr add 10.7.0.9/24 dev tun66

#sleep 5

#ip rule add to 10.7.0.9 iif lo              table tun33

#ip route del 10.7.0.9 dev tun66 table local
#ip route add 10.7.0.9 dev tun66 table local
#ip route del 10.0.1.1 dev tun33 table local
#ip route add 10.0.1.1 dev tun33 table local
#ip route del 10.0.2.1 dev tun34 table local
#ip route add 10.0.2.1 dev tun34 table local
#ip route del 10.0.3.1 dev tun35 table local
#ip route add 10.0.3.1 dev tun35 table local

#ip route flush cache


#ip rule add to 10.7.0.9 iif lo              table tun33
#ip route delete table local 10.0.1.1
#ip route delete table local 10.0.2.1
#ip route delete table local 10.0.3.1

#ip rule add table 252 priority 2
#ip route add local 10.7.0.9 dev tun66 proto kernel scope host src 10.7.0.9 table 252
#ip route add local 10.0.1.1 dev lo proto kernel scope host src 10.0.1.1 table 252
#ip route add local 10.0.2.1 dev lo proto kernel scope host src 10.0.2.1 table 252
#ip route add local 10.0.3.1 dev lo proto kernel scope host src 10.0.3.1 table 252

sleep 1

wireshark &
