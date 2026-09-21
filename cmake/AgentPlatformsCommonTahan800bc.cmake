# CMake to build libraries and binaries in fboss/agent/platforms/common/tahan

# In general, libraries and binaries in fboss/foo/bar are built by
# cmake/FooBar.cmake

add_library(tahan800bc_platform_mapping
  fboss/agent/platforms/common/tahan800bc/Tahan800bcPlatformMapping.cpp
  fboss/agent/platforms/common/tahan800bc/oss/Tahan800bcPlatformMapping.cpp
)

target_link_libraries(tahan800bc_platform_mapping
  platform_mapping
)
