# RackmonD (x86 and BMC) - A FBOSS Platform Service

## Objective
1. RackmonD controls rack power module and rack battery modules. More specifically, it reads the hardware state and status of rack power devices, and return the reads through thrift interface, which is then consumed by Meta's DC infra servcies for managing rack power.


   * < BMC Classic System >

      META Infra <==REST==> BMC <==RS485==> RACK_POWER_DEVICES


   *  < BMC Lite System >

      META Infra <==Thrift==> x86 <==RS485==> RACK_POWER_DEVICES


## Design Concept
* In BMC-Classic platforms (Wedge400 and older), rackmond runs in BMC, while in BMC-Lite platforms (Darwin and later) rackmond runs in x86 CPU as system service.
* The service will read rack power modules, over RS485 bus (which is exactly same medium as RJ45 based lan/console cable), and return the reading through Thrift (BMC-Lite) or slower REST API (BMC-Classic)

## Usage
* The rackmond will kickstart automatically upon system start, and will restart upon any crash. So users do not need to worry about running it.
* On BMC-Classic system, users can use rackmonctl command to run debugging commands to see the list of attached power devices and their error rate
* On BMC-Lite system, currently no debugging command is provided, but the user can check the log to see what issue has occurred


## Trouble Shooting
* rackmond is attached to wrong devices, or the number of devices attached are less than expected. Is this software bug? : No it is not. The same SW runs for all racks and they works correctly in most of the racks. If you see some device is missing or wrong devices are seen, most of the time it is cabling issue. That is, the cable is connected incorrectly, or some of the cable is not working which make some of rack power devices not able to be discovered
