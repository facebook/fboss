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
