Olympic QoS
-----------

Olympic QoS model unifies QoS model across CBB/EBB/DC.


Quality of Service
-------------------

- During normal operation, there is enough network capacity to satisfy demands.
- Under congestion: the ingress packet rate is higher than port egresss rate.
  Thus, the switch must drop packets.
- Desired behavior: 'higher priority' flow drops < 'lower priority' flow drops.
- How is a flow identified: 6-bit DSCP fields in IP header.


Facebook QoS model - Olympic QoS
--------------------------------

Defines class of services as follows:

- Bronze: Cost optimized traffic (e.g. coldstorage, backups)
- Silver: General infrastructure traffic (e.g. data warehouse, dev infra, scribe)
- Gold: FB Family minimum viable product (e.g. messaging, newsfeed)
- ICP: Server Infra control plane (e.g. SMC, zeus, MySQL, TAO)
- Network Control: Network Control Plane (e.g. BGP, OpenR)

Defines which DSCP point(s) map to which Olympic QoS queue.


FBOSS Olympic QoS configuration
--------------------------------

The below table lists Olympic QoS configuration


+-----------+-----+------+------+-----------------+-------+---------------------+---------+
| Queue Name| Type| #DSCP|Weight|Reserved Bytes   | Alpha |     DSCP codes      | QueueID |
+-----------+-----+------+------+-----------------+-------+---------------------+---------+
|  Bronze   | WRR |  24  |  5   | 0               |  1    | [10-11], [16-17]    |    4    |
|           |     |      |      |                 |       | [19-23], [25]       |         |
|           |     |      |      |                 |       | [50-63]             |         |
+-----------+-----+------+------+-----------------+-------+---------------------+---------+
|  Silver   | WRR |  23  |  15  | - 0 RSW         |  1    | [0-9], [12-15],     |    0    |
|           |     |      |      | - 3328 FA, FSW  |       | [40-47], [49]       |         |
+-----------+-----+------+------+-----------------+-------+---------------------+---------+
|  Gold     | WRR |   9  |  80  | - 0 RSW         |  8    | [18], [24], [31],   |    1    |
|           |     |      |      | - 9984 FA, FSW  |       | [33-34], [36, 39]   |         |
+-----------+-----+------+------+-----------------+-------+---------------------+---------+
|  ICP      | SP  |   7  |  NA  | - 0 RSW         |  8    | [26-30], [32], [35] |    6    |
|           |     |      |      | - 9984 FA, FSW  |       |                     |         |
+-----------+-----+------+------+-----------------+-------+---------------------+---------+
| Network   | SP  |   1  |  NA  |  0              |  NA   | [48]                |    7    |
| Control   |     |      |      |                 |       |                     |         |
+-----------+-----+------+------+-----------------+-------+---------------------+---------+

WRR: Weighted Round Robin
SP: Strict Priority

SP traffic has higher priority than WRR and can starve WRR out. If SP traffic
does not consume all the link capacity, the available link capacity is used
amongst the contending WRR queues in proportion to their weights.

QoS for MH-NIC RSW downlinks
----------------------------

RSW downlinks of MH-NIC configuration have Queue-per-host. Refer
(:doc:`mhnic_fix`) for additional details.

QoS for non-MH-NIC RSW downlinks
--------------------------------

RSW downlinks have different QoS profile:

+-----------+-----+------+
| Queue Name| Type|Weight|
+-----------+-----+------+
|  Bronze   | WRR |  1   |
+-----------+-----+------+
|  Silver   | WRR |  16  |
|           |     |      |
+-----------+-----+------+
|  Gold     | WRR |  32  |
+-----------+-----+------+
|  ICP      | WRR |  64  |
+-----------+-----+------+
| Network   | SP  |  NA  |
| Control   |     |      |
+-----------+-----+------+


- RSW downlinks have the same DSCP to queue mapping as Olympic QoS policy.
- Non-downlink ports on RSW have the same Olympic QoS policy as described earlier in this document.
- Note that ICP is SP in Olympic QoS model, but WRR for RSW downlinks.
- Why is RSW downlink configuration different:

  - Multiple servers (typically 4) could connect to a Multi-host NIC, which in
    turn connects to a single RSW port.
  - With ICP as SP, it was observed that a sender sending ICP to one
    of the 4 servers could starve other queues, effectively starving other
    servers connected to the same multi-host NIC.
  - ICP has been converted to WRR to mitigate this problem.
