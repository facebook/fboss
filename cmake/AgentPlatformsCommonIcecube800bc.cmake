# CMake to build libraries and binaries in fboss/agent/platforms/common/tahan

# In general, libraries and binaries in fboss/foo/bar are built by
# cmake/FooBar.cmake

add_library(icecube800bc_platform_mapping
  fboss/agent/platforms/common/icecube800bc/Icecube800bcPlatformMapping.cpp
)

target_link_libraries(icecube800bc_platform_mapping
  platform_mapping
)
