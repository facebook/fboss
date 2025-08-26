# CMake to build libraries and binaries in fboss/agent/platforms/common/wedge800ba

# In general, libraries and binaries in fboss/foo/bar are built by
# cmake/FooBar.cmake

add_library(wedge800ba_platform_mapping
  fboss/agent/platforms/common/wedge800ba/Wedge800baPlatformMapping.cpp
)

target_link_libraries(wedge800ba_platform_mapping
  platform_mapping
)
