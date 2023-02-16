# CMake to build libraries and binaries in fboss/agent/platforms/common/yangra

# In general, libraries and binaries in fboss/foo/bar are built by
# cmake/FooBar.cmake

add_library(yangra_platform_mapping
  fboss/agent/platforms/common/yangra/YangraPlatformMapping.cpp
)

target_link_libraries(yangra_platform_mapping
  platform_mapping
)
