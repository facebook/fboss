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

add_library(hw_port_fb303_stats
  fboss/agent/hw/HwPortFb303Stats.cpp
)

target_link_libraries(hw_port_fb303_stats
  counter_utils
  fb303::fb303
  FBThrift::thriftcpp2
  hardware_stats_cpp2
  Folly::folly
)
