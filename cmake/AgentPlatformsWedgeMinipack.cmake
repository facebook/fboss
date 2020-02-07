# CMake to build libraries and binaries in fboss/agent/platforms/wedge/minipack

# In general, libraries and binaries in fboss/foo/bar are built by
# cmake/FooBar.cmake

add_library(minipack_platform_mapping
  fboss/agent/platforms/wedge/minipack/Minipack16QPimPlatformMapping.cpp
  fboss/agent/platforms/wedge/minipack/oss/MinipackPlatformMapping.cpp
)

target_link_libraries(minipack_platform_mapping
  platform_mapping
)
