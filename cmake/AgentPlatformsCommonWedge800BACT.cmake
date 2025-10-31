# CMake to build libraries and binaries in fboss/agent/platforms/common/wedge800bact

# In general, libraries and binaries in fboss/foo/bar are built by
# cmake/FooBar.cmake

add_library(wedge800bact_platform_mapping
  fboss/agent/platforms/common/wedge800bact/Wedge800BACTPlatformMapping.cpp
)

target_link_libraries(wedge800bact_platform_mapping
  platform_mapping
)
