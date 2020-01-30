# CMake to build libraries and binaries in fboss/agent/hw/test

# In general, libraries and binaries in fboss/foo/bar are built by
# cmake/FooBar.cmake

add_library(config_factory
  fboss/agent/hw/test/ConfigFactory.cpp
)

target_link_libraries(config_factory
  fboss_types
  hw_switch
  switch_config_cpp2
  Folly::folly
)

add_library(hw_test_main
  fboss/agent/hw/test/Main.cpp
)

target_link_libraries(hw_test_main
  fboss_init
  Folly::folly
)

add_library(hw_link_state_toggler
  fboss/agent/hw/test/HwLinkStateToggler.cpp
)

target_link_libraries(hw_link_state_toggler
  core
  switch_config_cpp2
  state
  Folly::folly
)

add_library(hw_switch_ensemble
  fboss/agent/hw/test/HwSwitchEnsemble.cpp
)

target_link_libraries(hw_switch_ensemble
  hw_link_state_toggler
  core
)

add_library(hw_switch_test
  fboss/agent/hw/test/HwLinkStateDependentTest.cpp
  fboss/agent/hw/test/HwTest.cpp
  fboss/agent/hw/test/HwTestConstants.cpp
  fboss/agent/hw/test/HwTestStatUtils.cpp
  fboss/agent/hw/test/HwTestCoppUtils.cpp
  fboss/agent/hw/test/HwRouteScaleTest.cpp
  fboss/agent/hw/test/HwRouteOverflowTest.cpp
  fboss/agent/hw/test/HwVlanTests.cpp
  fboss/agent/hw/test/HwL2ClassIDTests.cpp
)

target_link_libraries(hw_switch_test
  config_factory
  hw_switch_ensemble
  core
  hardware_stats_cpp2
  route_distribution_gen
  route_scale_gen
  Folly::folly
)
