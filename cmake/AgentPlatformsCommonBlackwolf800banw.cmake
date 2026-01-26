# CMake to build libraries and binaries in fboss/agent/platforms/common/blackwolf800banw

# In general, libraries and binaries in fboss/foo/bar are built by
# cmake/FooBar.cmake

add_library(blackwolf800banw_platform_mapping
  fboss/agent/platforms/common/blackwolf800banw/Blackwolf800banwPlatformMapping.cpp
)

target_link_libraries(blackwolf800banw_platform_mapping
  platform_mapping
)
