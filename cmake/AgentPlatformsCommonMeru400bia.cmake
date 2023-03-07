# CMake to build libraries and binaries in fboss/agent/platforms/common/meru400bia

# In general, libraries and binaries in fboss/foo/bar are built by
# cmake/FooBar.cmake

add_library(meru400bia_platform_mapping
  fboss/agent/platforms/common/meru400bia/Meru400biaPlatformMapping.cpp
)

target_link_libraries(meru400bia_platform_mapping
  platform_mapping
)
