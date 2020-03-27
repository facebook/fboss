# CMake to build libraries and binaries in fboss/agent/platforms/wedge/wedge400

# In general, libraries and binaries in fboss/foo/bar are built by
# cmake/FooBar.cmake

add_library(wedge400c_platform_mapping
    fboss/agent/platforms/common/wedge400c/Wedge400CPlatformMapping.cpp
)

target_link_libraries(wedge400c_platform_mapping
  platform_mapping
)
