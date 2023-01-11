# CMake to build libraries and binaries in fboss/agent/platforms/wedge/wedge400

# In general, libraries and binaries in fboss/foo/bar are built by
# cmake/FooBar.cmake

add_library(cloud_ripper_platform_mapping
    fboss/agent/platforms/common/cloud_ripper/CloudRipperPlatformMapping.cpp
    fboss/agent/platforms/common/cloud_ripper/CloudRipperVoqPlatformMapping.cpp
    fboss/agent/platforms/common/cloud_ripper/CloudRipperFabricPlatformMapping.cpp

)

target_link_libraries(cloud_ripper_platform_mapping
  platform_mapping
)
