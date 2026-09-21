# Ping Test

## Steps

1. Connect the DUT to another host with at least one link
    1. The other host can be any device, not necessarily FBOSS device.
2. Generate agent config based on cable connection and assign IPs to connected interfaces
    1. All agent config settings are the same as link test, except that we need to assign IP to the L3 interfaces connected to the other device.
    2. For example, say port eth1/1/1 is connected to the other host. This port is put in vlan interface 2001, with assigned IP addresses "2400::/64" and "10.0.0.0/24".
    ```json
   "ports": [
     {
       "logicalID": 9,
       "state": 2,
       "minFrameSize": 64,
       "maxFrameSize": 9412,
       "parserType": 1,
       "routable": true,
       "ingressVlan": 2001,
       "speed": 400000,
       "name": "eth1/1/1",
       ...
     },
     ...
   ]

   "interfaces": [
     {
       "intfID": 2001,
       "routerID": 0,
       "vlanID": 2001,
       "ipAddresses": [
         "2400::/64",
         "10.0.0.0/24"
       ],
       ...
     },
     ...
   ]
3. Generate QSFP config
4. Build FBOSS qsfp\_service binary
5. Build FBOSS wedge\_agent binary
6. Bring up qsfp\_service on both devices
7. Bring up wedge\_agent on both devices
8. Verify link connected to the other host is up
    1. “fboss2 show port” to verify port up
    2. “fboss2 show interface” to verify correct IP assigned
9. Verify ping works between DUT and the other host
    1. just use `ping <remote host IP>` command in Linux
    2. Some use fboss2 commands to help debugging:
        1. “fboss2 show mac details” to verify mac entry to the other host is learned
        2. “fboss2 show ndp” to verify ndp entry to the other host is learned
        3. "fboss2 show arp"
        4. “fboss2 start|stop pcap” to start and stop capturing all control packets sent to/received by FBOSS control plane
