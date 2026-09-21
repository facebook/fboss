# CMake to build libraries and binaries in fboss/agent/platforms/common/saintpaul

# In general, libraries and binaries in fboss/foo/bar are built by
# cmake/FooBar.cmake

add_library(saintpaul_platform_mapping
  fboss/agent/platforms/common/saintpaul/SaintpaulPlatformMapping.cpp
)

target_link_libraries(saintpaul_platform_mapping
  platform_mapping
)
