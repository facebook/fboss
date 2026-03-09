# CMake to build libraries and binaries in fboss/agent/platforms/common/yangra2

# In general, libraries and binaries in fboss/foo/bar are built by
# cmake/FooBar.cmake

add_library(yangra2_platform_mapping
  fboss/agent/platforms/common/yangra2/oss/Yangra2PlatformMapping.cpp
)

target_link_libraries(yangra2_platform_mapping
  platform_mapping
)
