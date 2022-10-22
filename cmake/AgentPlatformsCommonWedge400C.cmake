# CMake to build libraries and binaries in fboss/agent/platforms/wedge/wedge400

# In general, libraries and binaries in fboss/foo/bar are built by
# cmake/FooBar.cmake

add_library(wedge400c_platform_mapping
    fboss/agent/platforms/common/wedge400c/Wedge400CPlatformMapping.cpp
    fboss/agent/platforms/common/wedge400c/Wedge400CGrandTetonPlatformMapping.cpp
    fboss/agent/platforms/common/wedge400c/Wedge400CVoqPlatformMapping.cpp
    fboss/agent/platforms/common/wedge400c/Wedge400CFabricPlatformMapping.cpp
)

target_link_libraries(wedge400c_platform_mapping
  platform_mapping
)


add_library(wedge400c_ebb_lab_platform_mapping
    fboss/agent/platforms/common/ebb_lab/Wedge400CEbbLabPlatformMapping.cpp
)

target_link_libraries(wedge400c_ebb_lab_platform_mapping
  platform_mapping
)

add_library(wedge400c_platform_utils
  fboss/agent/platforms/common/wedge400c/oss/Wedge400CPlatformUtil.cpp
)
