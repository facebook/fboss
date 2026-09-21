Warm Boot and ECMP shrink
=========================
This document describes the various link transition events that can occur during warm boot
and the impact on traffic.

Warm boot overview
------------------
Warm boot feature provides a method for upgrading wedge_agent software without impacting
traffic flow on the switch. This is achieved by saving software and hardware state
information before shutdown and restoring the state on warm boot restart.

ECMP shrink
-----------
If a link transitions to oper down state, control plane will take a finite
amount of time to reconverge and reroute the traffic through the remaining links.
Data plane will forward traffic on down link till convergence happens. ECMP shrink
feature reduces the duration of traffic loss during link down events by quickly
removing the failed member from ECMP. The ECMP path will be updated by control plane
at a later point of time when software converges.

Warm boot and link events
-------------------------
If a port transitions to oper down, link scan thread will update all ECMPs which
have a member egressing on the down port. In addition, it will update the switch
state to mark the port down so that protocols such as ARP and ND can react to
the port event. Routing protocol will converge at a later point and remove the
next hops going over the failed link. A warm boot shut down can happen during This
period resulting in a couple of scenarios.

Case 1 : Warm boot shut down happens after ECMP shrink and ARP/ND removal but
before routing convergence
Case 2 :  Warm boot shut down happens after ECMP shrink but before ARP/ND
removal and routing convergence

During warm boot bring up, there are again two scenarios.
Port down - The port oper state remains down during init
Port up - The port oper state is up during init. This can happen if the link
fault clears during the time period when software was not running.

When wedge agent shuts down during warm boot upgrade, it saves the sw switch state
in a persistant memory. In addition, wedge agent saves hw switch (Bcm switch) state
information for ECMP. The ECMP entries in both sw switch and hw switch
contains members that were removed as part of ECMP shrink.
During warm boot init, a map of egress id list to ECMP id is created using the
hw switch information. The key to this list contains the member removed by ECMP
shrink because the route entries are still using the original nexthop list.
Later when sw switch ECMP entries are restored, the above mentioned map is used
to locate the ECMP entry. The hardware ECMP membership is not modified at this
point and there will not be any traffic loss.

At a later point in warm boot init, the neighbor entries get restored. In Case 1
listed previously, this will be an no-op since neighbor entries are removed. However
in Case 2, the newly added egress id will be added to ECMP objects though the port is down.
This happens becuase every time a neighbour entry is resolved, agent adds the egress id
to all ECMPs that egress id was previously a part of. However this addition will be
short lived since at the end of warm boot init, all port states will be checked
and paths resolving over down ports will be removed. This traffic loss scenario
should be very rare since we expect ARP/ND entries to be removed in most cases.

The following table summarizes the behavior when a port transitions to down state
and routing hasn't converged before warm boot shutdown.
+ -------------------+------------------------+-------------------+
| Port state at init |     Down               |    Up             |
+--------------------+------------------------+-------------------+
| ARP/ND updated     |   No traffic loss      |   No traffic loss |
+--------------------+------------------------+-------------------+
| ARP/ND not updated |   Short traffic loss   |   No traffic loss |
+--------------------+------------------------+-------------------+


How to detect traffic loss case?
- Check for the following debug message in wedge agent log to see whether a member
was added to ECMP during warm boot init.

V0924 22:48:47.585746 2713656 BcmEgress.cpp:581] Added 100099 to 200256 before transitioning to INIT state
