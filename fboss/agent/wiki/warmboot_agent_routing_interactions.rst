Wedge Agent and Routing protocol interactions during Warmboot
=============================================================

This document describes interactions between routing protocols and wedge agent during warm boot restart

Agent Warm boot Init Sequence
-----------------------------
Wedge agent implements a software switch(SwSwitch) and maintains a switch state object to represent the forwarding state. In addition, there is a hardware switch (BcmSwitch, SAISwitch) which manages the ASIC specific resources. The SwSwitch transitions through the following stages during a warm boot restart. It maintains a SwitchRunState variable that clients can query to identify the current state.

UNINITIALIZED:
This is the default SwitchRunState. Agent does basic SDK init and starts feature managers in this stage. During warm boot exit, agent stores hardware and software state information into a file on disk - /dev/shm/fboss/warm_boot/switch_state.
At init stage, it reads the contents of a warmboot file and restores the state information. In addition, it checks for any link events during warm boot phase and takes corrective action. No route add or delete updates from clients are allowed in this stage.

INITIALIZED:
Agent applies initial configuration(from /etc/coop/agent/current or user provided config FLAG) and changes SwitchRunState to CONFIGURED. The file contains knobs to enable or disable features, port and interface configs as well as various feature configs that are applied at this stage. The config update will not result in any state change unless config changed between warmboots or there were some events during warm boot agent down time. Any requests from routing client to add or delete routes are denied during this time period.

CONFIGURED:
Route add or delete requests from clients are allowed at this stage. When requests from clients are recieved, agent adds or deletes the corresponding routes in RIB and FIB. In addition FIB constructs a new switch state with updated routes and attempts to transition to the new switch state. A state delta is created as part of this operation and HwSwitch does create/delete/modify operations in hardware for the affected routes. If hardware switch is able to successfully apply the delta, switch transitions to the new state.

Protocol interactions
---------------------
BGP and OPENR are the two main clients that agent serves. A restart of agent results in restart of BGP and OPENR as well. After restart, routing protocols establish sessions with neighbors and relearn the routes. These routes could be different from the routes already programmed in hardware and hence need to be resynced. BGP and OPENR clients differ slightly in their initial interactions after warmboot.

BGP:
BGP waits for agent to transition to CONFIGURED SwitchRunState before syncing the routes using a syncFib command. This is a special route action command that includes all routes that are currently valid. Agent performs the following actions on the routes that belong to the client in response to the syncFib command.
 - All routes included in syncFib command and also present in agent state are preserved
 - If a route exists in syncFib but not in agent state, it is added to agent state
 - Any route present in agent state but missing in syncfib command is deleted.

 syncFib command ensures that any route update that might have happened during warm boot upgrade window are correctly synced between client and agent. After the initial syncFib, BGP issues route add and delete commands to manage the routes. If any of these commands fails, it will issue a new syncFib command to synchronize agent and protocol state.

OPENR:
After OpenR restart, it waits for 90 seconds before issuing a syncFib command. During this time period, it will install static MPLS label routes for prepend labels. A prepend label provides loop free forwarding for spine egress traffic. For spine routes, FSWs have direct connectivity to SSW. In addition other pods in the mesh will also announce the same route and this can lead to a forwarding loop. To avoid loop, if BGP learned spine route is the best path, OpenR will not install the mesh routes. However it installs a prepend label on local FSW with nexthops resolving towards turbo mesh members. Prepend label on remote side will be programmed such that traffic will not be sent back to mesh links. Prepend labels are computed before initial best path selection and hence will be programmed before fibSync.

 This will be followed by initial route computation and a syncFib and syncFibMpls command. After this, OPENR will add/remove routes similar to BGP case.
