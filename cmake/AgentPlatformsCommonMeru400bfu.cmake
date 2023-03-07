# CMake to build libraries and binaries in fboss/agent/platforms/common/meru400bfu

# In general, libraries and binaries in fboss/foo/bar are built by
# cmake/FooBar.cmake

add_library(meru400bfu_platform_mapping
  fboss/agent/platforms/common/meru400bfu/Meru400bfuPlatformMapping.cpp
)

target_link_libraries(meru400bfu_platform_mapping
  platform_mapping
)
