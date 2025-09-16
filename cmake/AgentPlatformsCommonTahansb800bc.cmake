# CMake to build libraries and binaries in fboss/agent/platforms/common/tahansb800bc

# In general, libraries and binaries in fboss/foo/bar are built by
# cmake/FooBar.cmake

add_library(tahansb800bc_platform_mapping
  fboss/agent/platforms/common/tahansb800bc/Tahansb800bcPlatformMapping.cpp
)

target_link_libraries(tahansb800bc_platform_mapping
  platform_mapping
)
