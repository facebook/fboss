# CMake to build libraries and binaries in fboss/agent/platforms/common/icetea800bc

# In general, libraries and binaries in fboss/foo/bar are built by
# cmake/FooBar.cmake

add_library(icetea800bc_platform_mapping
  fboss/agent/platforms/common/icetea800bc/Icetea800bcPlatformMapping.cpp
)

target_link_libraries(icetea800bc_platform_mapping
  platform_mapping
)
