# CMake to build libraries and binaries in fboss/agent/platforms/wedge/wedge400

# In general, libraries and binaries in fboss/foo/bar are built by
# cmake/FooBar.cmake

add_library(sandia_platform_mapping
    fboss/agent/platforms/common/sandia/Sandia8DDPimPlatformMapping.cpp
    fboss/agent/platforms/common/sandia/Sandia16QPimPlatformMapping.cpp
    fboss/agent/platforms/common/sandia/oss/SandiaPlatformMapping.cpp
)

target_link_libraries(sandia_platform_mapping
  platform_mapping
)
