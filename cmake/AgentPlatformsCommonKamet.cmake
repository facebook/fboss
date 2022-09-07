# CMake to build libraries and binaries in fboss/agent/platforms/common/kamet

# In general, libraries and binaries in fboss/foo/bar are built by
# cmake/FooBar.cmake

add_library(kamet_platform_mapping
  fboss/agent/platforms/common/kamet/KametPlatformMapping.cpp
)

target_link_libraries(kamet_platform_mapping
  platform_mapping
)
