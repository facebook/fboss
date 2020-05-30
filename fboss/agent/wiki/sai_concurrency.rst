SAI Concurrency Design Notes
============================
This document intends to provide a record of the thinking and discussion that
went into the design of concurrent access in SaiSwitch. Though it is a
relatively simple and naive scheme, there are some important considerations
that developers in the code base should be aware of.

Introduction
============
HwSwitch is heavily driven by a single thread in SwSwitch: the update thread.
Since that thread does the vast majority of the heavy lifting inside SaiSwitch,
our goal is to maintain the illusion that all interesting work in SaiSwitch
happens on that thread. Unfortunately, that is not completely compatible with
reality, and there are some actual concurrent computations happening. We receive
packets and various event notifications from the SAI adapter, and also attempt
to service packet tx, stats collection, and thrift requests from SwSwitch. We
enumerate those use cases in detail in an appendix below.

Since the use cases are generally limited in scope, we don't aim to make every
component of SaiSwitch highly parallel, we just need to poke enough holes in the
single-threaded illusion to allow for the efficient implementation of that
handful of concurrent computations.

Design evaluation
=================
We considered several designs for managing concurrency in SaiSwitch:
1. A global lock, with special data available in concurrency safe data
   structures (e.g. hazard pointer, rcu, CoW, etc..)
2. All data stored in concurrency safe data structures, design in concert to
   make the whole of SaiSwitch thread safe.
3. Granular locking carefully arranged to make all of SaiSwitch thread safe.
4. Chunking of high throughput, latency insensitive update work to allow for
   preemption by low throughput latency sensitive concurrent work.

We ruled out 2. and 3. for being overkill, as discussed above. We don't seek
arbitrary thread safety of any arbitrary operations on SaiSwitch without taking
into account the actual usage patterns. We started with 1. but ruled it out,
because it is difficult to safely support latency sensitive work which needs to
modify internal data structures (specifically link down handling). We chose to
pursue 4. as it is in the spirit of 1. with an explicit design for handling
changes to managers from off the update thread.

For implementing 4., we also considered a few different mechanisms:
a. Use folly::coro to structure the chunked work as coroutines and then
   implement cooperative multitasking with all SaiSwitch computations running
   on a single thread. AKA "greenthreads"
b. Similar to a. but schedule traditional lambdas on a priority queue enabled
   executor to run on a single thread. (Note: since EventBase does not have a
   notion of priority, we would have to use a new Executor, and our research
   suggested this would also require using folly::Futures, as executors in 
   general do not have EventBase's rich APIs for scheduling work.)
c. Maintain a single mutex for SaiSwitch, but give chunked computations a
   reference to it so that they can unlock between chunks to allow other work
   to grab the lock and proceed.

Ultimately, we chose 4.c over 4.a and 4.b because we felt that it was the
simplest, and used the best available scheduler for interleaving the serialized
work. Letting the OS scheduler ensure fairness between the work vying for the
mutex seems more robust than ensuring we generate a good interleaving of chunks
and also maintaining the scheduling primitives themselves (whether it be
cooperative coroutines or prioritized futures).

As coroutines mature, and more use-cases and patterns for cooperative
multitasking arise, it may make sense to revisit them as an option. It would
be attractive, from the perspective of simplicity and correctness, to have all
work truly running on a single thread.

Appendix: Enumeration of Concurrent Accesses
============================================
* stateChanged
  * thread: fbossUpdateThread
  * duration: unbounded
  * perf demand: high throughput
  * access type: RW
  * action: program SwitchState to hardware
  * data accessed: all
* linkStateChanged
  * thread: saiLinkScanBH
  * duration: short
  * perf demand: low latency
  * access type: RW
  * action: given sai port id, remove next hop group members using that id from next hop groups
  * data accessed:
    * port, next hop (via neighbor?), next hop group (at least)
* Fdb Event Notification
  * thread: fdbEventBH
  * duration: short
  * perf demand: low latency
  * action: program learned macs to fdb
  * access type: RW
  * action: given sai fdb learned object input, create an fdb entry -- possibly for reuse by Neighbor later
  * data accessed: fdb, neighbor (?)
* RX
  * thread: saiRXBH
  * duration: short
  * perf demand: low latency
  * access type: RO
  * action: given sai port id, fill out port, vlan metadata on SwSwitch rx packet
  * data accessed: sai port id -> PortID, VlanID
* AsyncTX
  * thread: saiAsyncTx
  * duration: short
  * perf demand: low latency
  * access type: RO
  * action: given PortID, call pipeline bypass tx function with right port sai id
  * data accessed: PortID -> sai port id
* Thrift
  * thread: thrift handler threads
  * duration: short
  * perf demand: low latency
  * access type: RO
  * action: get various internal SaiSwitch data
  * data accessed: all
* Stats
  * thread: stats collector thread
  * duration: short
  * perf demand: can run successfully on 1m periodic schedule (low latency?)
  * access type: RO (except clearing stats, so SaiApi write)
  * action: get/clear stats about the switch
  * data accessed: stats from a variety of apis
* Shell
  * thread: ReplThread
  * duration: unbounded
  * perf demand: "whatever" (but low latency)
  * access type: RW -- "unsafe"
  * action: run arbitrary commands at any level of the sdk interface
  * data accessed: more than all
