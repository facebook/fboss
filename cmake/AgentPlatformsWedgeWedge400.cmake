# CMake to build libraries and binaries in fboss/agent/platforms/wedge/wedge400

# In general, libraries and binaries in fboss/foo/bar are built by
# cmake/FooBar.cmake

add_library(wedge400_platform_mapping
  fboss/agent/platforms/wedge/wedge400/Wedge400PlatformMapping.cpp
)

target_link_libraries(wedge400_platform_mapping
  platform_mapping
)
