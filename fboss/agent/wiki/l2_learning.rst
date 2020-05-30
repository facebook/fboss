L2 Learning
===========

This document describes L2 Learning modes used in FBOSS viz. HARDWARE and SOFTWARE.

HARDWARE L2 Learning
--------------------

In this mode, wedge_agent is *not* involved in Learning/Aging and is thus unware
of the MACs learned/aged. Specifically, SwitchState->vlan's MAC table remains
empty.

Learning Workflow:

- When ASIC sees a packet with unknown source MAC+vlan (L2 table miss) on a
  port, it adds entry to L2 table for this MAC+vlan and port.
- Using the L2 entry, subsequent packets to this MAC+vlan are switched to that
  port.

Aging workflow:

- SDK runs a thread that ages out L2 entries.


SOFTWARE L2 Learning
--------------------

In this mode, wedge_agent gets call back on Learning/Aging. Specifically,
SwitchState->vlan's MAC table reflects ASIC L2 table.

However, there are some differences depending on the ASIC.


SOFTWARE L2 Learning: TD2, TH
------------------------------

Learning Workflow:

- With SOFTWARE L2 Learning enabled, when ASIC sees a packet with unknown
  source MAC (L2 table miss), it adds PENDING entry for this MAC+vlan and
  generates a callback.
- PENDING entry means that subsequent traffic with same source MAC+vlan does
  not result into further callbacks.
- Traffic to PENDING entry is still treated as unknown unicast and would be flooded.
- BCM layer receives callback with PENDING L2 entry and passes it up to SwSwitch.
- SwitchState's vlan maintains MacTable. SwSwitch adds this L2 entry to
  corresponding MAC Table and schedules a state update.
- The MAC table delta processing in BCM layer will program the MAC on to the
  ASIC. The L2 entry is now VALIDATED and packets to this MAC+vlan would no
  longer be flooded but switched to the port in L2 entry.

Aging Workflow:

- SDK runs a thread that ages out stale L2 entries.
- When L2 entry is aged out, it generates a callback to BCM layer, which in
  turn passes the callback up to SwSwitch.
- SwSwitch removes the MAC from SwitchState->vlan's MAC Table and schedules a
  state update.
- MAC table delta processing in BCM layer would attempt to remove the MAC, but
  in our current model, since MACs is already aged out, this is a no-op.


SOFTWARE L2 Learning: TH3
-------------------------

There is no notion of PENDING for TH3.

Learning Workflow:

- With SOFTWARE L2 Learning enabled, when ASIC sees a packet with unknown
  source MAC (L2 table miss), it adds entry for this MAC+vlan and
  generates a callback.
- Unlike TD2, TH, this is *not* a PENDING entry, thus subsequent traffic with
  same source MAC+vlan is switched to the port.
- BCM layer receives callback with VALIDATED L2 entry and passes it up to SwSwitch.
- SwitchState's vlan maintains MacTable. SwSwitch adds this L2 entry to
  corresponding MAC Table and schedules a state update.
- The MAC table delta processing in BCM layer will program the MAC on to the
  ASIC. The L2 entry is already VALIDATED, so this is a no-op.

Aging Workflow:

- This wofkflow is same as Aging Workflow for "SoftwareL2 Learning: TD2, TH"
