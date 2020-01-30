# CMake to build libraries and binaries in fboss/agent/rib

# In general, libraries and binaries in fboss/foo/bar are built by
# cmake/FooBar.cmake

add_library(standalone_rib
  fboss/agent/rib/ConfigApplier.cpp
  fboss/agent/rib/Route.cpp
  fboss/agent/rib/RouteNextHop.cpp
  fboss/agent/rib/RouteNextHopEntry.cpp
  fboss/agent/rib/RouteNextHopsMulti.cpp
  fboss/agent/rib/RouteTypes.cpp
  fboss/agent/rib/RouteUpdater.cpp
  fboss/agent/rib/RoutingInformationBase.cpp
)

target_link_libraries(standalone_rib
  network_to_route_map
  address_utils
  error
  fboss_types
  switch_config_cpp2
  ctrl_cpp2
  label_forwarding_action
  state_utils
  Folly::folly
)

add_library(network_to_route_map
  fboss/agent/rib/NetworkToRouteMap.h
)

target_link_libraries(network_to_route_map
  radix_tree
)

add_library(fib_updater
  fboss/agent/rib/ForwardingInformationBaseUpdater.cpp
)

target_link_libraries(fib_updater
  network_to_route_map
  standalone_rib
  fboss_types
  state
  Folly::folly
)
