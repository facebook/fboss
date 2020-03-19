# CMake to build libraries and binaries in fboss/agent/platforms/wedge/yamp

# In general, libraries and binaries in fboss/foo/bar are built by
# cmake/FooBar.cmake

add_library(galaxy_platform_mapping
  fboss/agent/platforms/common/galaxy/GalaxyFCPlatformMapping.cpp
  fboss/agent/platforms/common/galaxy/GalaxyLCPlatformMapping.cpp
)

target_link_libraries(galaxy_platform_mapping
  platform_mapping
  Folly::folly
)
