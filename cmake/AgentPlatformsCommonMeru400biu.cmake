# CMake to build libraries and binaries in fboss/agent/platforms/common/meru400biu

# In general, libraries and binaries in fboss/foo/bar are built by
# cmake/FooBar.cmake

add_library(meru400biu_platform_mapping
  fboss/agent/platforms/common/meru400biu/Meru400biuPlatformMapping.cpp
)

target_link_libraries(meru400biu_platform_mapping
  platform_mapping
)
