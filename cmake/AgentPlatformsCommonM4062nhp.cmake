# CMake to build libraries and binaries in fboss/agent/platforms/common/m4062nhp

# In general, libraries and binaries in fboss/foo/bar are built by
# cmake/FooBar.cmake

add_library(m4062nhp_platform_mapping
  fboss/agent/platforms/common/m4062nhp/M4062nhpPlatformMapping.cpp
)

target_link_libraries(m4062nhp_platform_mapping
  platform_mapping
)
