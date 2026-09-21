# CMake to build libraries and binaries in fboss/agent/platforms/common/elbert

# In general, libraries and binaries in fboss/foo/bar are built by
# cmake/FooBar.cmake

add_library(elbert_platform_mapping
  fboss/agent/platforms/common/elbert/oss/ElbertPlatformMapping.cpp
  fboss/agent/platforms/common/elbert/Elbert16QPimPlatformMapping.cpp
)

target_link_libraries(elbert_platform_mapping
  platform_mapping
)
