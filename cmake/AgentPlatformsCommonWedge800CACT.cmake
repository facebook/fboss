# CMake to build libraries and binaries in fboss/agent/platforms/common/wedge800cact

# In general, libraries and binaries in fboss/foo/bar are built by
# cmake/FooBar.cmake

add_library(wedge800cact_platform_mapping
  fboss/agent/platforms/common/wedge800cact/Wedge800CACTPlatformMapping.cpp
)

target_link_libraries(wedge800cact_platform_mapping
  platform_mapping
)
