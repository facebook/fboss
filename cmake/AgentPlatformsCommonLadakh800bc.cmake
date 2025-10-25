# CMake to build libraries and binaries in fboss/agent/platforms/common/ladakh800bc

# In general, libraries and binaries in fboss/foo/bar are built by
# cmake/FooBar.cmake

add_library(ladakh800bc_platform_mapping
  fboss/agent/platforms/common/ladakh800bc/Ladakh800bcPlatformMapping.cpp
)

target_link_libraries(ladakh800bc_platform_mapping
  platform_mapping
)
