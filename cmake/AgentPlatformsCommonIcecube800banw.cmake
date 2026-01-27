# CMake to build libraries and binaries in fboss/agent/platforms/common/icecube800banw

# In general, libraries and binaries in fboss/foo/bar are built by
# cmake/FooBar.cmake

add_library(icecube800banw_platform_mapping
  fboss/agent/platforms/common/icecube800banw/Icecube800banwPlatformMapping.cpp
)

target_link_libraries(icecube800banw_platform_mapping
  platform_mapping
)
