# CMake to build libraries and binaries in fboss/agent/platforms/wedge/wedge400

# In general, libraries and binaries in fboss/foo/bar are built by
# cmake/FooBar.cmake

add_library(wedge400_platform_mapping
  fboss/agent/platforms/common/wedge400/Wedge400PlatformMapping.cpp
  fboss/agent/platforms/common/wedge400/Wedge400AcadiaPlatformMapping.cpp
  fboss/agent/platforms/common/wedge400/Wedge400GrandTetonPlatformMapping.cpp
)

target_link_libraries(wedge400_platform_mapping
  platform_mapping
)

add_library(wedge400_platform_utils
  fboss/agent/platforms/common/wedge400/oss/Wedge400PlatformUtil.cpp
)  
