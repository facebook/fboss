# CMake to build libraries and binaries in fboss/agent/platforms/common/montblanc

# In general, libraries and binaries in fboss/foo/bar are built by
# cmake/FooBar.cmake

add_library(montblanc_platform_mapping
  fboss/agent/platforms/common/montblanc/MontblancPlatformMapping.cpp
)

target_link_libraries(montblanc_platform_mapping
  platform_mapping
)
