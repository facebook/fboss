# CMake to build libraries and binaries in fboss/agent/platforms/wedge

# In general, libraries and binaries in fboss/foo/bar are built by
# cmake/FooBar.cmake

add_library(platform
  fboss/agent/platforms/wedge/FakeWedge40Platform.cpp
  fboss/agent/platforms/wedge/GalaxyFCPlatform.cpp
  fboss/agent/platforms/wedge/GalaxyLCPlatform.cpp
  fboss/agent/platforms/wedge/GalaxyPort.cpp
  fboss/agent/platforms/wedge/Wedge100Platform.cpp
  fboss/agent/platforms/wedge/Wedge100Port.cpp
  fboss/agent/platforms/wedge/Wedge40Platform.cpp
  fboss/agent/platforms/wedge/Wedge40Port.cpp
  fboss/agent/platforms/wedge/WedgeLedCode.cpp
  fboss/agent/platforms/wedge/WedgePlatform.cpp
  fboss/agent/platforms/wedge/WedgePlatformInit.cpp
  fboss/agent/platforms/wedge/WedgePort.cpp
  fboss/agent/platforms/wedge/WedgeTomahawkPlatform.cpp
  fboss/agent/platforms/wedge/oss/GalaxyPlatform.cpp
  fboss/agent/platforms/wedge/oss/GalaxyPort.cpp
  fboss/agent/platforms/wedge/oss/Wedge40Port.cpp
  fboss/agent/platforms/wedge/oss/Wedge100Port.cpp
  fboss/agent/platforms/wedge/oss/WedgePlatform.cpp
  fboss/agent/platforms/wedge/oss/WedgePlatformInit.cpp
)

target_link_libraries(platform
  bcm
  product_info
  fboss_config_utils
  qsfp_cpp2
  Folly::folly
)
