# CMake to build libraries and binaries in fboss/agent/platforms/common/makalu

# In general, libraries and binaries in fboss/foo/bar are built by
# cmake/FooBar.cmake

add_library(makalu_platform_mapping
  fboss/agent/platforms/common/makalu/oss/MakaluPlatformMapping.cpp
)

target_link_libraries(makalu_platform_mapping
  platform_mapping
)
