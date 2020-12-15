Implementing Transactions on SaiSwitch
==========================================


Requirements
-------------

We have a requirement to suffer a subset of errors without crashing. The use cases
fall into 2 buckets

#. Table overflow during a object add or update(e.g. route, neighbor). 
#. Config rollback (or in general a complete SwitchState rollback).
    
The implementation in BcmSwitch only handles 1 and uses scope guards to locally 
undo work when such an error is triggered (with additional complication for computing 
and returning the correct SwitchState). Further the BcmSwitch is highly tailored to
routes and host objects and does not lend it self to easy extension. We aim to come
up with a more general design for SAI. Secondly the design aims to be maximally 
effecient in the no failure case. 

Consider the following 2 modes for transactions

#. Overflow protection - Will apply to routes (IP, MPLS) and neighbors for now but can be 
   extended to other resources. For such objects, each object will be applied in sequence,
   stopping at the first failure. Any state created as a result of this overflowing object
   will be cleaned up and the actual applied switch state will be returned to higher layers.
   https://github.com/facebook/fboss/blob/master/fboss/agent/HwSwitch.h#L153
   Overflow protection is always on post HwSwitch transition to CONFIGURED state,  modulo 
   switch state transactions (described next)
#. SwitchState transaction - This applies to a use case where entire StateDelta must be 
   applied as a whole. Such StateDeltas maybe conveyed down as transaction by means 
   of a flag. Now when a failure occurs we try to rollback to previous switch state
   or oldSwitchState in StateDelta parlance - https://github.com/facebook/fboss/blob/master/fboss/agent/state/StateDelta.h#L81
   For SwitchState transactions, we will handle only a subset of errors, as a starting
   point, we may only consider table overflow error. 

Currently on SwitchState transactions are implemented in SaiSwitch code

What is needed for rolloback
-------------------------------
If we step back and look at SaiSwitch from the prism of SwitchState application. To implement 
transactions we fundamentally need a way to **rollback** to a known good SwitchState. For SwSwitch
this rollback simply means changing appliedState pointer to this known good SwitchState. For
SaiSwitch however it implies restoring all its data structures to reflect this known good
SwitchState. So if we could clear out all the data structures that reflect the broken 
SwitchState and make them reflect the known good switch state we would be done. Implicit here
is the fact that rollback should be minimally (in most cases not at all) disruptive to forwarding
else otherwise we might as well have done cold boot to a known good state. 
Finally note that For SaiSwitch, nearly (if not) all of SwitchState application is reflected 
in manager classes in switch directory. 

Piggybacking on the warm boot mechanism
----------------------------------------

Consider the problem statement - make SaiSwitch reflect a known good state with mimimal disruption
to forwarding. This is eerily similar to warmboot. And would be great if we could reuse some of
the same logic. 
Consider the following general scenario. Say we have two switch states - SwitchStateGood, 
SwitchStateFailed. Where failure occurred somewhere in the process of getting from 
SwitchStateGood to SwitchStateFailed. Our goal is to restore SaiSwitch to SwitchStateGood.
Now we do the following.

#. Acquire SaiSwitch mutex - https://github.com/facebook/fboss/blob/master/fboss/agent/hw/sai/switch/SaiSwitch.h#L343
#. Block Hw writes from this thread (extending the mechanism here - https://github.com/facebook/fboss/blob/master/fboss/agent/hw/sai/api/SaiApi.h#L74)
#. Clear out SaiManagerTable https://github.com/facebook/fboss/blob/master/fboss/agent/hw/sai/switch/SaiSwitch.h#L351
#. Unblock HW writes
#. Now perform steps simialr to what we do during WB here - https://github.com/facebook/fboss/blob/master/fboss/agent/hw/sai/switch/SaiSwitch.cpp#L837-L874 and here - https://github.com/facebook/fboss/blob/master/fboss/agent/hw/sai/switch/SaiSwitch.cpp#L197-L234 (SaiSwitchManager may need soome tweaking since we don't want to do a actual warmboot from a adapter's perspective). Essentially what we are doing above is this 
    * Populate SaiStore with all objects in HW
    * Apply state delta of StateDelta(EmptySwitchState, SwitchStateGood)
    * As during WB, anything matched in SaiStores would not require going to the HW
    * Post SwitchStateGood application, we clean up any warmboot handles remaining in SaiStore. Goal there being to clean up any HwState left over in going from SwitchStateGood to SwitchStateFailed
#. Return SwitchStateGood as the applied state back to caller. 


Tracking SwitchStateGood
--------------------------
Going back to the 2 use cases described at the begining - overflow protection and SwitchState
transation. In both these cases we need to know the SwitchState to which we want to restore 
SaiSwitch. For the SwitchState transaction, this is quite simple, just retore to the old state
in StateDelta. For the overflow case, I am thinking of the following (ordered by preference)

#. Track iter in delta application and then in case of a failed delta (say RouteTableDelta)
   pass back this information to the caller. The caller then uses it to construct SwitchStateGood.
   Thus modifications here https://github.com/facebook/fboss/blob/master/fboss/agent/hw/sai/switch/SaiSwitch.cpp#L1367-L1418
   and here - https://github.com/facebook/fboss/blob/master/fboss/agent/state/DeltaFunctions.h#L57-L108
   Note that iterators would need to be tracked for all SwitchState nodes. This should not be a
   big deal given that not too many leve1 nodes in SwitchState https://github.com/facebook/fboss/blob/master/fboss/agent/state/SwitchState.h
#. If the above proves expensive, we have one more trick in our toolbox. 
   Say we want to get from SwitchState A to SwitchState C and failure occurs 
   at SwitchState B. Now we can do the following
    * Do no tracking till a failure occurs. 
    * On failure use the piggybacking on WB mechanism to get to A. But don't clean out the
      SaiStore yet. 
    * Turn on tracking. Now failure should again occur at B (ignore background ALPM shuffle,
      although this mechanism handles that too, since failure would just occur at B'). 
    * Do a second pass of piggybacking on WB to get to B'. 
    * Clean up HW and return appliedState up.

Caveats
--------
Note that these proposed operations are quite costly in the event of a
transaction rollback. For this reason, it is important to not typically trigger rollbacks,
nor to get in a state where we repeatedly trigger them on the same switch after
it gets into some kind of "almost full" state.

Alternatives considered
------------------------
* Use HwResourceStats to preemptively reject about to fail updates - https://github.com/facebook/fboss/blob/master/fboss/agent/hw/hardware_stats.thrift#L73-L137
  This is tempting but unfortunately does not work. Most notably resource stats assume exclusive use, viz. if we added on /64 routes, how many could be added. This means we can't use them to evaulate 
  a enitre state delta. We would thus need to refresh these (from HW) after each node application and evaluate whether the next node can be applied or not. Secondly, not all resources have resource
  counters, meaning this design coukd come to a halt when we try to protect aginst overflow of next ressource. 
* Rely on SaiSwitch's use of ref map to do roll back. This will not help us in rolling back to a previous SwitchState. Secondly it retains all the problems of tracking to what extent a state was applied.

Open questions
---------------
#. For overflow, should we continue applying the other tables. Viz, if routes overflow, 
   continue applying the neighbor, fdb etc tables. 
#. For now, just restrict the protection to routes?. For BCM we added hosts since it had
   the host route in host table optimization, which is not the case for SAI. 
#. We need to revisit the mechanism of percolating error back to external callers - openr,
   bgp etc. 


