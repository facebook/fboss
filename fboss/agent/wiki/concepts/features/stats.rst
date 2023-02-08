Switch Statistics
=================

Feature Description
====================

Overview
=========

Sometimes when debugging detailed networking problems or tracking new
statistics, understanding the low-level details of how statistics
are generated and tracked across the FBOSS agent are important. In
particular, for performance reasons, collection of switch statistics are
done periodically (e.g., once per second, one per minute, etc.) and you
need to know how the statistics are collected to understand what they're
actually telling you.

Design
=======

Many of the statistics can update at full line rate (>10 billion
packets per second) which is faster than even the beefiest CPUs can
handle. So the Broadcom chips (really, all packet forwarding ASICs)
expose statistics as hardware registers for the software agents (e.g.,
FBOSS) to read periodically.

FBOSS launches a thread 'updateCounters' to read the hardware statistics
once per second and stores the values for thrift/fb303. As a result,
making thrift/fb303 calls to the switch to read a counter faster than
once per second is useless, because internally the counter is only
updating from the hardware once per second.

Unfortunately, there is an additional complication. At the
bottom of the updateCounters call stack is a Broadcom API call,
e.g., bcm_cosq_stat_get(unit, port, queue, type, &value ) or
bcm_stat_get(unit, port, &value). where critically, the 'value'
here is typically a 64 bit value. In practice, this is itself a layer
of abstraction because some of the underlying hardware counters are not
64-bit values.

For counters that are 64-bit, the SDK function calls directly pull from
the hardware registers (read: this is the simple case).

However, for the purpose of saving bits, particularly the port stats
counters (e.g., rx packets received), it's too expensive from a hardware
design standpoint to have a 64 bit counter for each port for each type
of counter (unicast, broadcast, dropped, etc.). As a result, some of the
underlying hardware counters are smaller than 64 bits. To maintain the
abstraction of a 64 bit counter, internally the SDK itself launches its
own statics tracking thread that periodically copies from the hardware
counter.

This SDK thread performs the following algorithm:

- reads $value from the underlying hardware counter (e.g., an 32 bit counter)
- if $value is less than $old_value, declare a counter wrap (assume a single counter wrap!): $virtual_value += 2^32
- Adds the updated value to a 64 bit $virtual_value region: $virtual_value += $value - $old_value Note: if there was a counter wrap, $value minus $old_value will be negative but will still do the right thing.
- $old_value = $value

and the SDK call returns the $virtual_value.

The Broadcom 'bcm_stat_interval' SDK parameter sets that period (in
milliseconds) though FBOSS's bcm.conf does not change it from its default
value of 1 second:

$ fboss_bcm_shell localhost --command 'counter'

Current settings: Interval=1000000 <--- this is in micro seconds
PortBitMap=0x000000000000000000000000000000000000000000000001ffffffffffffffff,
cpu,xe DMA=True For more information, check out the ./doc/counter.txt
file in any SDK source directory!

In theory, we could remove the thread polling step and directly DMA
our counters into the region of memory used by agent to export to
fb303. Whether this would have value or not is TBD, but good to know
it's possible.

Design: State Propagation
==========================

- Packet arrives and hardware counter is immediately incremented
- Broadcom SDK thread periodically copies from hardware register (<64 bit) into main memory 64-bit counter: currently 1/second
- FBOSS agent periodically calls a 'get stat' SDK call to copy from the SDK memory into memory in agent that's referenced in thrift/fb303 calls: currently 1/second
- fbagent periodically calls getCounters() on agent to get list of counters and then copies all counter data into its memory:currently 1/minute
- ODS periodically calls into fbagent to pull counters into ODS's long term time series storage: currently every 1-5 minutes depending on counter configuration


Design: Timing Implications for ~1 Second Monitoring
=====================================================

Given the under the covers implementation and the interaction between
the Broadcom SDK periodic counter thread and the FBOSS thread counter,
it's possible for a packet that's counted by the hardware at time t=0
to not show up in fb303 getCounter until 2 full seconds later, e.g.,
if we just missed the Broadcom SDK counter thread scheduling (1 second
per update) and that thread is just slightly out of sync with the FBOSS
updateCounters thread (also 1 second). For right now, that's probably
ok, but it would be easy to tweak (e.g., by reducing either of these
intervals) and perhaps even possible to remove (e.g., by going to DMA).

Scale
======

N/A


Use Case: Statistics Runbook for a Single Switch
=================================================

All FBOSS statistics are exported via the fb303 interface (running on
agent's thrift port - 5909), so any of the following will work:

- Get a list of statistics/counters we track from a switch (e.g., "rsw1ao.21.snc1"): fb303c getCounters -s rsw1ao.21.snc1:5909
- Grab a specific statistic (e.g., "cpu.queue2.in_pkts.sum") once a second: watch -n1 fb303c -s rsw1ao.21.snc1:5909 getCounter cpu.queue2.in_pkts.sum
- Grab a graphical view of counters and current values: 'bunnylol fb303 rsw1ao.21.snc1' in your favorite webbrowser
- View stats as a graphical time series: 'bunnylol fb303/graph' in your favorite webbrowser

Note that when trying to view statistics across multiple switches,
particularly over time, using ODS is probably a better tool to view
this information.


Use Case: Exporting Statistics Off Box (e.g., to ODS)
======================================================

Similar to other Facebook services, once statistics are available from
agent via fb303, then fbagent (not to be confused with wedge_agent) polls
the statistics from agent once per minute. Then, ODS polls from fbagent
typically once every one to five minutes, depending on the counter. So
the question of "where do I look at the statistics" comes down to "how
fresh do you need the data".

Configuration
==============

The feature is enabled by default, no configuration knobs available.

Build and test
===============

N/A

Debug
======

N/A

Sample Output
==============

fb303c getCounters -s rsw001.p006.f02.snc1:5909 | grep warm_boot
  "warm_boot.configured.stage_duration_ms": 1197,
  "warm_boot.fib_synced_bgp.stage_duration_ms": 2625,
  "warm_boot.fib_synced_openr.stage_duration_ms": 1757,
  "warm_boot.initialized.stage_duration_ms": 8670,
  "warm_boot.parent_process_started.stage_duration_ms": 18040,
  "warm_boot.process_started.stage_duration_ms": 10000,
  "warm_boot.shutdown.stage_duration_ms": 6666,
  "warm_boot.total_duration_ms": 44573,

watch -n1 fb303c -s rsw001.p006.f02.snc1:5909 getCounter warm_boot.fib_synced_openr.stage_duration_ms
Every 1.0s: fb303c -s rsw001.p006.f02.snc1:5909 getCounter warm_boot.fib_synced_openr.stage_duration_ms

rsw001.p006.f02.snc1:5909       1757
