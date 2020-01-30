# CMake to build libraries and binaries in fboss/agent

# In general, libraries and binaries in fboss/foo/bar are built by
# cmake/FooBar.cmake

add_library(fboss_types
  fboss/agent/types.cpp
)

target_link_libraries(fboss_types
  switch_config_cpp2
  Folly::folly
)

add_library(hw_switch
  fboss/agent/HwSwitch.cpp
)

target_link_libraries(hw_switch
  fboss_types
  ctrl_cpp2
  fboss_cpp2
  Folly::folly
)
