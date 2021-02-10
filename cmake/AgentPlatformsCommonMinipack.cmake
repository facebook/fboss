# CMake to build libraries and binaries in fboss/agent/platforms/common/minipack

# In general, libraries and binaries in fboss/foo/bar are built by
# cmake/FooBar.cmake

add_library(minipack_platform_mapping
  fboss/agent/platforms/common/minipack/Minipack16QPimPlatformMapping.cpp
  fboss/agent/platforms/common/minipack/oss/MinipackPlatformMapping.cpp
)

target_link_libraries(minipack_platform_mapping
  platform_mapping
)
