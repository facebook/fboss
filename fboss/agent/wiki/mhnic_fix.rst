MH-NIC Fix
==========

The primary goal of this text is to document FBOSS design for MH-NIC fix.

First two sections introduce the Problem and Solution in brief and then the
rest of the text details FBOSS design.

Problem
-------

MH-NIC hardware connects multiple hosts but plugs into single RSW port. RSW is
unaware that multiple hosts sit behind single port, Thus, the egress queue on RSW
port gets shared between the MH-NIC hosts. As a result, it is possible for
traffic to one MH-NIC host to adversely affect traffic to other MH-NIC hosts,
even if the traffic belongs to the same traffic class (same DSCP value).

Solution
--------

For RSW downlink ports that can connect to MH-NICs (e.g. Yosemite racks)
dedicate a queue for each host. This isolates traffic to MH-NIC hosts from each
other. As a result, Olympic QoS is disabled for such ports.

FBOSS solution outline
----------------------

At a high level, the FBOSS solution involves following aspects:

- static configuration: queue configuration, ACLs
- dynamic configuration: discovering connected MACs, neighbors and associating those with unique queues

Static configuration
--------------------

This consists of following:

- One queue for each host MH-NIC can connect to + 1 for oob. For example, 

    Yosemite racks = 4 MH-NIC hosts + 1 oob = 5,
    Thor MH-NIC    = 2 MH-NIC hosts + 1 oob = 3
- Each queue has identical configuration: Weighted round robin, same bandwidth limit. For example,

   Yosemite racks = 50Gbps / 4 MH-NIC hosts = 12.5Gbps,
   Thor MH-NIC    = 100Gbps / 2 MH-NIC hosts = 50Gbps
- L2 as well as L3 ACLs to associate unique classID with each queue. More on this later in the text.
- Enable SOFTWARE L2 Learning. Refer (:doc:`l2_learning`) for additional details.


Dynamic Configuration
---------------------

This consists of discovering connected MACs/neighbors/routes and associating
classIDs with those so that packets matching those MACs/neighbors/routes are
processed by appropriate queue.

ClassID and ACLs
----------------

ClassID is an abstraction that helps glue MACs/neighbors with queues. The
statically created ACLs match on classID and process traffic matching certain
classID with pre-chosen queue.
MACs/neighbors are discovered dynamically, and we associate classIDs with those
for the ACLs to kick in and provide queue-per-host semantics.


Type of traffic/originator matters
----------------------------------

From a solution perspective, we want to provide queue-per-host semantics
regardless of the type/originator of the traffic. However, from the
implementation perspective, there are different cases:

- Traffic to MH-NIC host from another host connected to same MH-NIC: L2 within MH-NIC
- Traffic to MH-NIC host from another host connected to same RSW: L2 within RSW
- Traffic to MH-NIC host from outside the RSW: L3 (host entry)
- Traffic to MH-NIC host from outside the RSW where MH-NIC host is next hop: L3 (route entry)

L2 within MH-NIC
----------------

This traffic is not seen by RSW, and thus FBOSS does not provide queue-per-host
solution for this traffic. In theory, it is possible to force this traffic to
get routed so that it hits RSW and then RSW can provide queue-per-host
semantics. However, we don't implement this today, and at this point in time,
we haven't seen the need to solve the noisy neighbor problem for this scenario.


L2 within RSW
-------------

The solution consists of associating classID with L2 entries as MACs are learned.

- MAC Learning or aging results in update to SwitchState->vlan's MAC table.
  For additional details, refer :doc:`l2_learning`).
- LookupClassUpdater is implemented as a state observer, and thus observes MAC
  Learning or aging state changes.
- In response, LookupClassUpdater associates/disassociates classID with MAC entry
  and schedules a state update.
- BCM layer processing of this state update will associate/disassociate classID
  with L2 entry.
- If the number of unique MACs exceeds the number of classIDs available, we
  wrap around. However, for a given configuration, we provision enough queues
  (and classIDs) so in practice, we won't need to wrap around.
  For example, on yosemite setup which has 4 MH-NIC hosts + 1 oob, we program
  5 queues, each with a unique classID, and in practice, we would get no more
  than 5 unique MACs that we would need to associate classIDs for.

L3 (host entry)
---------------

The solution consists of associating classID with host entries when neighbors
are resolved.

- Neighbor resolve/unresolve results in update to SwitchState->vlan's ARP or NDP table.
- LookupClassUpdater is implemented as a state observer, and thus observes
  neighbor entries getting resolved/unresolved (or deletes).
- In response, LookupClassUpdater associates/disassociates classID with neighbor entry
  and schedules a state update.
- BCM layer processing of this state update will associate/disassociate classID
  with host entry and schedule state update.
- As described in L2 within RSW section, classID assignment would wrap around if needed,
  but that won't happen in practice.

L3 (route entry)
----------------

The solution consists of associating classID with route entries when next hop
for the route entries are resolved.

- LookupClassUpdater is implemented as a state observer, and uses helper class
  LookupClassRouteUpdater. LookupClassRouteUpdater processes route updates.
- When a route is added, if the route's next hop (neighbor) is resolved and
  next hop has a classID associated with it, associate the same classID with
  the route entry and schedule a state update.
- BCM layer processing of this state update will associate classID with route
  entry and schedule state update.
- If the route's next hop (neighbor) is not yet resolved, maintain a reverse
  map of next hop (key) and list of routes (values). Later, if/when the next
  hop is resolved and gets a classID, walk the list of routes and associate
  classID of the next hop with each of the routes and schedule a state update.
- Similarly, when a neighbor is unresolved (and loses classID), use the above
  map to remove classID associated with every route whose next hop is this
  neighbor.

Optimization:

Switch state's route node contains route to nexthop information. When a
neighbor is resolved, we could walk the current switch state to discover all
routes whose nexthop is the neighbor that was just resolved. While it works,
this brute force approach is inefficient as it involves walking all
(potentially large) number of routes for each neighbor resolve/unresolve.

Thus, the implementation maintains a reverse map: next hop (key) and list of
routes (values). Furthermore, note that the Noisy Neighbor problem and thus its
Queue-per-host fix is only applicable for downlink ports. Thus, the map need
not cache every next hop to corresponding routes. It only needs to cache for
the routes that can exist on downlink ports (well known subnet) which is
significantly smaller.

Limitation:

A route entry has an egress object. The egress object can point to an ECMP
group or a physical port. A classID could be associated with a route entry
entry but not with an egress object. Thus, a packet hitting route gets same
classID (associated with route) regardless of which member of the ECMP group
(if any) gets chosen to egress the packet.

In theory, it is possible for route to get classID 12 (say, and thus egress
through queue 2). This route could point to an ECMP group whose members are
MH-NIC hosts in the same sled. In this scenario, regardless of which MH-NIC
host the traffic goes to, it would use queue2.

Choosing classID
----------------

LookupClassUpdater keeps track of the number times a classID is used, and
chooses the 'least used classID' while assigning classID for a MAC.
Note that this logic runs per port.


Refcounting
-----------

- When LookupClassUpdater observes a new MAC, it assigns a classID to it. This
  MAC could be result of MAC table changes (L2 within RSW) or neighbor table
  changes (L3 - host entry) etc, and increments the the reference count.
- Subsequent classID assiginment requests for the same MAC will return the same classID
  but increment the reference counter. A typical case would involve MAC table
  change prompting classID assignment to a MAC, and shortly after, the neighbor
  resolution will request classID for the same MAC.
- Conversely, MAC aging/neighbor unresolve will decrement the reference count
  but the classID would be released to free pool only after the reference count
  drops to 0.
- This model means that both L2 as well as L3 traffic gets same classID, and
  thus goes to same queue, and yet, we need not be concerned about the order in
  which learning/aging/resolve/unresolve happens to guarantee this behavior.

Why SW L2 Learning
------------------

An alternate approach would have been as follows

- Use HW L2 Learning, thus no callbacks on MAC Learning or aging.
- When ARP/NDP resolves, the L3 portion of the fix chooses a classID and
  associates it with corresponding host entry.
- At that time, we also know the MAC address, and could associate classID with
  the L2 entry thereby providing queue-per-host fix for L2.

This has several downsides:

- The approach does not work for pure L2 traffic.
- If L2 entry were to age out before ARP/NDP, the L2 traffic will cause the MAC
  entry to be relearnt, however, no classID would be associated with the
  traffic till ARP/NDP re-resolves, thus breaking queue-per-host fix.
- From a design perspective, we end up with two entities controlling the L2
  table viz. the HW learning in ASIC and the wedge_agent logic that associates
  classID. It is cleaner to drive programming of L2 table centrally using
  wedge_agent.

Given all the above, we choose to go with SW L2 Learning to implement
queue-per-host fix.

QoS within Queue-per-host queues
--------------------------------

In the current implementation, all traffic egress from RSW to one MH-NIC host
gets one queue. This queue does not distinguish between different Olympic
traffic types. The traffic within single queue is serviced on first come first
serve basis.

In theory, it is possible to use multiple ‘colored’ profiles associated with
the same queue to approximate Olympic within single queue-per-host queue. We
might experiment that in the future.

Monitoring: configuration, stats
--------------------------------

- fboss -H $switchName hosts # QueueID: queue-per-host queue or Olympic
- fboss -H $switchName port details # queue config, queue stats

Consider a neighbor IP N1 has MAC M1. Also, consider a route R1 whose nexthop is
N1. Each of neighbor entry N1 (ARP/NDP entry), L2 entry M1 and route entry R1
have the same classID associated. This is listed by fboss -H $switchName hosts
Queue ID column which is implemented by displaying classID associated with
neighbor IP. However, for debugging, it is possible to query classID individiually:

- fboss -H $switchName l2 table
- fboss -H $switchName arp table
- fboss -H $switchName ndp table

Monitoring: feature enablement
------------------------------

When queue-per-host + DCTCP is enabled, each queue-per-host queue has
2-profiles viz.:

- ECN marking profile for ECT marked traffic
- Drop profile for non-ECT marked traffic

Check if a switch has queue-per-host or queue-per-host + DCTCP enabled:

- smcc tiers $switchName | grep fboss.feature.queue_per_host.on
- smcc tiers $switchName | grep fboss.feature.queue_per_host_dctcp.on

All switches with queue-per-host or queue-per-host + DCTCP enabled:

- smcc ls-hosts -r fboss.feature.queue_per_host.on
- smcc ls-hosts -r fboss.feature.queue_per_host_dctcp.on
