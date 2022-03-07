# CMake to build libraries and binaries in fboss/agent/platforms/wedge/wedge400

# In general, libraries and binaries in fboss/foo/bar are built by
# cmake/FooBar.cmake

add_library(sandia_platform_mapping
    fboss/agent/platforms/common/sandia/SandiaPlatformMapping.cpp
)

target_link_libraries(sandia_platform_mapping
  platform_mapping
)
