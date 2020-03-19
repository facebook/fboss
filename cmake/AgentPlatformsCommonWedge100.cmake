# CMake to build libraries and binaries in fboss/agent/platforms/wedge/wedge400

# In general, libraries and binaries in fboss/foo/bar are built by
# cmake/FooBar.cmake

add_library(wedge100_platform_mapping
  fboss/agent/platforms/common/wedge100/Wedge100PlatformMapping.cpp
)

target_link_libraries(wedge100_platform_mapping
  platform_mapping
)
