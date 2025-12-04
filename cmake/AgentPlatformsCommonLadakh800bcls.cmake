# CMake to build libraries and binaries in fboss/agent/platforms/common/ladakh800bcls

# In general, libraries and binaries in fboss/foo/bar are built by
# cmake/FooBar.cmake

add_library(ladakh800bcls_platform_mapping
  fboss/agent/platforms/common/ladakh800bcls/Ladakh800bclsPlatformMapping.cpp
)

target_link_libraries(ladakh800bcls_platform_mapping
  platform_mapping
)
