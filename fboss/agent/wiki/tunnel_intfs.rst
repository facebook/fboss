Tunnel interfaces
=================

Tunnel interfaces (TUN) are virtual network interfaces (implemented entirely in
software) that operate with layer 3 packets like IP Packets.

FBOSS: Management vs. inband
============================

From network hardware perspective, an FBOSS switch has:

- A physical NIC
- Broadcom ASIC

Physical NIC
------------

- Shows up as eth0
- ethtool -i eth0  # returns corresp. driver e.g. igb
- lspci | grep <NIC manufacturer> e.g. I210 Gigabit Network Connection (rev 03)
- this is Management interface.
- ping6/ssh to switch uses this
  For example, run ip addr show eth0 from the switch and,
  ping6 rsw1ay.07.ftw1.facebook.com from outside the switch
  The global IPv6 address in ip addr show output matches the IP address used by
  ping6.

Broadcom ASIC
-------------

- lspci | grep Broadcom # e.g. BCM56960 on wedge100
- FBOSS wedge_agent programs this ASIC to provide switching/routing
  functionality

Tunnel interfaces: reaching switch over Broadcom ASIC
------------------------------------------------------

- fboss10 is a tunnel interface created by FBOSS wedge_agent.
- Inband ping uses this IP address
  For example, run ip addr show fboss10 from the switch and,
  ping6 rsw1ay-inband.07.ftw1.facebook.com from outside the switch
  The global IPv6 address in ip addr show output matches the IP address used by
  ping6.
- the main use case is BGP to send packets to peers.

Need for Management vs. inband
------------------------------

- The management interface (eth0) is typically connected to a non-FBOSS switch.
- The management interface provides out-of-band (without using FBOSS) way to
  connect to an FBOSS switch.
- This is useful for debugging in production as well as development: as FBOSS bug
  cannot lock us out of network access to the switch.
