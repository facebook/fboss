# CMake to build libraries and binaries in fboss/agent/platforms/common/m5120csc

# In general, libraries and binaries in fboss/foo/bar are built by
# cmake/FooBar.cmake

add_library(m5120csc_platform_mapping
  fboss/agent/platforms/common/m5120csc/M5120CSCPlatformMapping.cpp
)

target_link_libraries(m5120csc_platform_mapping
  platform_mapping
)
