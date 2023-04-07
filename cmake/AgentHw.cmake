# CMake to build libraries and binaries in fboss/agent/hw

# In general, libraries and binaries in fboss/foo/bar are built by
# cmake/FooBar.cmake

add_library(counter_utils
  fboss/agent/hw/CounterUtils.cpp
)

target_link_libraries(counter_utils
  fb303::fb303
  FBThrift::thriftcpp2
  hardware_stats_cpp2
  switch_config_cpp2
)

add_library(diag_cmd_filter
  fboss/agent/hw/DiagCmdFilter.cpp
)

target_link_libraries(diag_cmd_filter
  Folly::folly
)

add_library(hw_switch_fb303_stats
  fboss/agent/hw/HwSwitchFb303Stats.cpp
)

add_library(hw_fb303_stats
  fboss/agent/hw/HwFb303Stats.cpp
)

add_library(hw_port_fb303_stats
  fboss/agent/hw/HwBasePortFb303Stats.cpp
  fboss/agent/hw/HwPortFb303Stats.cpp
  fboss/agent/hw/HwSysPortFb303Stats.cpp
  fboss/agent/hw/oss/HwBasePortFb303Stats.cpp
)

add_library(hw_cpu_fb303_stats
  fboss/agent/hw/HwCpuFb303Stats.cpp
)

add_library(hw_trunk_counters
  fboss/agent/hw/HwTrunkCounters.cpp
)

add_library(hw_switch_warmboot_helper
  fboss/agent/hw/HwSwitchWarmBootHelper.cpp
)

add_library(buffer_stats
  fboss/agent/hw/BufferStatsLogger.cpp
)

add_library(hw_resource_stats_publisher
  fboss/agent/hw/HwResourceStatsPublisher.cpp
)

target_link_libraries(hw_switch_warmboot_helper
  async_logger
  utils
  common_file_utils
  Folly::folly
)

target_link_libraries(hw_switch_fb303_stats
  stats
  fb303::fb303
  Folly::folly
  common_utils
)

target_link_libraries(hw_fb303_stats
  counter_utils
  fb303::fb303
  Folly::folly
  switch_config_cpp2
)

target_link_libraries(hw_port_fb303_stats
  counter_utils
  FBThrift::thriftcpp2
  hardware_stats_cpp2
  Folly::folly
)

target_link_libraries(hw_cpu_fb303_stats
  counter_utils
  FBThrift::thriftcpp2
  hardware_stats_cpp2
  Folly::folly
  switch_config_cpp2
)

target_link_libraries(hw_trunk_counters
  counter_utils
  FBThrift::thriftcpp2
  hardware_stats_cpp2
  Folly::folly
  fboss_types
)

target_link_libraries(buffer_stats
  Folly::folly
)

target_link_libraries(hw_resource_stats_publisher
  fb303::fb303
  hardware_stats_cpp2
)
