# CMake to build libraries and binaries in fboss/agent/state

# In general, libraries and binaries in fboss/foo/bar are built by
# cmake/FooBar.cmake

add_library(state
  fboss/agent/state/AclEntry.cpp
  fboss/agent/state/AclMap.cpp
  fboss/agent/state/AggregatePort.cpp
  fboss/agent/state/AggregatePortMap.cpp
  fboss/agent/state/ArpEntry.cpp
  fboss/agent/state/ArpResponseTable.cpp
  fboss/agent/state/ArpTable.cpp
  fboss/agent/state/ControlPlane.cpp
  fboss/agent/state/ForwardingInformationBase.cpp
  fboss/agent/state/ForwardingInformationBaseContainer.cpp
  fboss/agent/state/ForwardingInformationBaseDelta.cpp
  fboss/agent/state/ForwardingInformationBaseMap.cpp
  fboss/agent/state/Interface.cpp
  fboss/agent/state/InterfaceMap.cpp
  fboss/agent/state/LabelForwardingEntry.cpp
  fboss/agent/state/LabelForwardingInformationBase.cpp
  fboss/agent/state/LoadBalancer.cpp
  fboss/agent/state/LoadBalancerMap.cpp
  fboss/agent/state/MacEntry.cpp
  fboss/agent/state/MacTable.cpp
  fboss/agent/state/MatchAction.cpp
  fboss/agent/state/Mirror.cpp
  fboss/agent/state/MirrorMap.cpp
  fboss/agent/state/NdpEntry.cpp
  fboss/agent/state/NdpResponseTable.cpp
  fboss/agent/state/NdpTable.cpp
  fboss/agent/state/NeighborResponseTable.cpp
  fboss/agent/state/NodeBase.cpp
  fboss/agent/state/Port.cpp
  fboss/agent/state/PortMap.cpp
  fboss/agent/state/PortQueue.cpp
  fboss/agent/state/QosPolicy.cpp
  fboss/agent/state/QosPolicyMap.cpp
  fboss/agent/state/Route.cpp
  fboss/agent/state/RouteDelta.cpp
  fboss/agent/state/RouteNextHop.cpp
  fboss/agent/state/RouteNextHopEntry.cpp
  fboss/agent/state/RouteNextHopsMulti.cpp
  fboss/agent/state/RouteTable.cpp
  fboss/agent/state/RouteTableMap.cpp
  fboss/agent/state/RouteTableRib.cpp
  fboss/agent/state/RouteTypes.cpp
  fboss/agent/state/RouteUpdater.cpp
  fboss/agent/state/SflowCollector.cpp
  fboss/agent/state/SflowCollectorMap.cpp
  fboss/agent/state/StateDelta.cpp
  fboss/agent/state/StateUtils.cpp
  fboss/agent/state/SwitchSettings.cpp
  fboss/agent/state/QcmConfig.cpp
  fboss/agent/state/SwitchState.cpp
  fboss/agent/state/Vlan.cpp
  fboss/agent/state/VlanMap.cpp
  fboss/agent/state/VlanMapDelta.cpp
)

target_link_libraries(state
  error
  platform_config_cpp2
  switch_config_cpp2
  switch_state_cpp2
  fboss_cpp2
  label_forwarding_action
  state_utils
  radix_tree
  phy_cpp2
  Folly::folly
)

add_library(state_utils
  fboss/agent/state/StateUtils.cpp
)

target_link_libraries(state_utils
  error
  fboss_types
)

add_library(label_forwarding_action
  fboss/agent/state/LabelForwardingAction.cpp
)

target_link_libraries(label_forwarding_action
  error
  fboss_cpp2
  Folly::folly
)
