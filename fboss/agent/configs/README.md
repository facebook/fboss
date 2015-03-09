### Sample Configs 

This directory contains some sample config files.  The data
layout is specified in `switch_config.thrift`


#### sample1.json
  * 64 ports
  * 1 vlan


#### sample2.json
  * 64 ports
  * 64 vlans (one per port)


#### sample3.json
  * 48 ports
  * 5 vlans (44 ports in 1 vlan (downlink), and 1 vlan for each of the remaining
  ports (uplinks))
  * 5 L3 interfaces (1 for each of the vlans)
