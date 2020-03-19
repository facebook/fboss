# CMake to build libraries and binaries in fboss/agent/platforms/wedge/wedge400

# In general, libraries and binaries in fboss/foo/bar are built by
# cmake/FooBar.cmake

add_library(wedge40_platform_mapping
  fboss/agent/platforms/common/wedge40/Wedge40PlatformMapping.cpp
)

target_link_libraries(wedge40_platform_mapping
  platform_mapping
)
