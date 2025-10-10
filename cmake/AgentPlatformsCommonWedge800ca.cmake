# CMake to build libraries and binaries in fboss/agent/platforms/common/wedge800ca

# In general, libraries and binaries in fboss/foo/bar are built by
# cmake/FooBar.cmake

add_library(wedge800ca_platform_mapping
  fboss/agent/platforms/common/wedge800ca/Wedge800caPlatformMapping.cpp
)

target_link_libraries(wedge800ca_platform_mapping
  platform_mapping
)
