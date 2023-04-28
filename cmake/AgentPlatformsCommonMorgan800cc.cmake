# CMake to build libraries and binaries in fboss/agent/platforms/morgan/morgan800cc

# In general, libraries and binaries in fboss/foo/bar are built by
# cmake/FooBar.cmake

add_library(morgan_platform_mapping
    fboss/agent/platforms/common/morgan/Morgan800ccPlatformMapping.cpp
)

target_link_libraries(morgan_platform_mapping
  platform_mapping
)
