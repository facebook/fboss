.. fbmeta::
   hide_page_title=true

ACLs
####

Feature Description
===================

ACLs Overview
-------------

An ACL entry consists of an Action that is performed if a packet matches
specified Matcher criteria.

For example, an ACL can drop (Action) all packets with specific source IP (Matcher).
A wide variety of Matchers (based on different packet fields, source/dst port etc.) and
Actions (drop, increment counter, modify packet contents etc.) are supported.

An ACL Table consists of multiple ACL entries. Only the first ACL entry
matching a packet takes effect and the rest of the ACL entries in the ACL Table
are ignored. In order to match on multiple ACL entries on the same packet, the
ACL entries must reside in separate ACL Tables.

An ACL Table Group consists of multiple ACL Tables.

ACLs Design
-----------

The current FBOSS implementation supports single ACL Table. Thus, a packet can
match only one ACL entry. We will support Multiple ACL Tables in future.

List of ACLs (which all belong to the single Table) are represented in
SwitchState. HwSwitch implementation processes delta for the list of ACLs and
programs hardware.

ACLs Use Case
-------------

- Copying certain traffic to CPU: this is typically control plane traffic that
  is copied to CPU, and egress via specific queue. For example, Matcher: traffic
  with linkLocal DstIP + DSCP 48, Action: Copy to CPU via egress queue 9.
- Queue-per-host ACLs: these are used to provide QoS. For example, Matcher:
  classID associated with a neighbor entry, Action: egress via specific front
  panel queue.

ACLs Configuration
==================

- List of ACL entries (struct AclEntry): each entry carries a name, and list of matchers.
- DataPlaneTrafficPolicy: list of actions e.g. egress via specific queue, mirror etc. for front panel ports.
- CPUTrafficPolicy: list of actions e.g. egress via specific queue, mirror etc. for CPU port.

ACL entry's 'name' is matched against MatchToAction.matcher to determine
corresponding matcher.

Build and Test
==============

- Unit tests: fboss/agent/state/tests/Acl*
- HwTests: fboss/agent/HwAcl*
