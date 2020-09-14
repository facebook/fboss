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
)


add_library(hw_switch_stats
  fboss/agent/hw/HwSwitchStats.cpp
)

add_library(hw_fb303_stats
  fboss/agent/hw/HwFb303Stats.cpp
)

add_library(hw_port_fb303_stats
  fboss/agent/hw/HwPortFb303Stats.cpp
  fboss/agent/hw/oss/HwPortFb303Stats.cpp
)

add_library(hw_cpu_fb303_stats
  fboss/agent/hw/HwCpuFb303Stats.cpp
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
  utils
  Folly::folly
)

target_link_libraries(hw_switch_stats
  stats
  fb303::fb303
  Folly::folly
)

target_link_libraries(hw_fb303_stats
  counter_utils
  fb303::fb303
  Folly::folly
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
)

target_link_libraries(buffer_stats
  Folly::folly
)

target_link_libraries(hw_resource_stats_publisher
  fb303::fb303
  hardware_stats_cpp2
)
