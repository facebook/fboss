# CMake to build libraries and binaries in fboss/agent/platforms/common/minipack3n

# In general, libraries and binaries in fboss/foo/bar are built by
# cmake/FooBar.cmake

add_library(minipack3n_platform_mapping
  fboss/agent/platforms/common/minipack3n/oss/Minipack3NPlatformMapping.cpp
)

target_link_libraries(minipack3n_platform_mapping
  platform_mapping
)
