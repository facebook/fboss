# CMake to build libraries and binaries in fboss/agent/platforms/common/j4sim

# In general, libraries and binaries in fboss/foo/bar are built by
# cmake/FooBar.cmake

add_library(j4sim_platform_mapping
  fboss/agent/platforms/common/j4sim/J4SimPlatformMapping.cpp
)

target_link_libraries(j4sim_platform_mapping
  platform_mapping
)
