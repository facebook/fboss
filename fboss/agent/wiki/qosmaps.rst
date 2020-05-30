QoS Maps
--------

QoS Maps is a Broadcom feature that allows specifying 'action' for packets
'matched' under certain criteria.

Use case
--------

Implementing QoS. Refer (:doc:`olympic_qos`) for additional details.


Why not use ACLs?
-----------------

ACLs can be used to implement QoS (and the initial implementation used ACLs
instead of QoS Maps). However, it has following limitations:

- ACLs are precious resouce: the max number is limited by hardware. Thus, an
  alternative mechanism that frees up ACL resouce is welcome.

- Owing to a Broadcom bug, the wedge_agent resets field processor when
  it restarts (cold boot as well as warmboot).  Thus, ACLs are not enforced
  for a short duration during agent restart.
  (The bug has since been reportedly fixed, so we may choose to not reset field
  processor during warmboot in the future, and in that case, this reason will no
  longer apply).

- If ACLs are used to implement Olympic QoS (refer: doc:`olympic_qos`), ACLs
  cannot be used for any other action without breaking QoS. Reason:

  - Each packet has a DSCP value (IP header field)
  - Olympic QoS model defines DSCP to queue mapping for every possible DSCP value.
  - Only 1 ACL can match a given packet. Thus, we either get QoS (if QoS ACL
    has higher priority) or any other ACL feature but no QoS (if QoS ACL has lower
    priority), but never both.


How do QoS Maps work?
---------------------


QoS Maps works as follows:

- Program a map<dscp, priority> in the HW and associate it with port(s).
- Using dscp value in a packet and above map, tag each packet with priority.
- Another map translates priority tag to CoQ index, and sends packets to that
  queue.


ACLs and QoS Maps
=================

If both ACLs and QoS Map defines a packet's queue mapping, ACL decision will
trump QoS Map decision.'
