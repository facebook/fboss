# CMake to build libraries and binaries in fboss/agent/platforms/wedge/yamp

# In general, libraries and binaries in fboss/foo/bar are built by
# cmake/FooBar.cmake

add_library(yamp_platform_mapping
  fboss/agent/platforms/common/yamp/Yamp16QPimPlatformMapping.cpp
  fboss/agent/platforms/common/yamp/YampPlatformMapping.cpp
)

target_link_libraries(yamp_platform_mapping
  platform_mapping
)
