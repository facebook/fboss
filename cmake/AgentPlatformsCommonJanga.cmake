# CMake to build libraries and binaries in fboss/agent/platforms/common/janga

# In general, libraries and binaries in fboss/foo/bar are built by
# cmake/FooBar.cmake

add_library(janga_platform_mapping
  fboss/agent/platforms/common/janga/JangaPlatformMapping.cpp
)

target_link_libraries(janga_platform_mapping
  platform_mapping
)
