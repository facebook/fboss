Agent daemon
------------

One of the central pieces of FBOSS is the agent daemon, which runs on each
switch, and controls the hardware forwarding ASIC. The agent daemon sends
forwarding information to the hardware, and implements following control plane
protocols: ARP, NDP, LACP, DHCP relay.

The agent provides thrift APIs for managing routes, to allow external routing
control processes to get their routing information programmed into the hardware
forwarding tables.
