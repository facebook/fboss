# CMake to build libraries and binaries in fboss/agent/platforms/wedge

# In general, libraries and binaries in fboss/foo/bar are built by
# cmake/FooBar.cmake

add_library(platform
  fboss/agent/platforms/wedge/WedgePlatform.cpp
  fboss/agent/platforms/wedge/WedgePlatformInit.cpp
  fboss/agent/platforms/wedge/WedgePort.cpp
  fboss/agent/platforms/wedge/WedgeTomahawkPlatform.cpp
  fboss/agent/platforms/wedge/galaxy/GalaxyFCPlatform.cpp
  fboss/agent/platforms/wedge/galaxy/GalaxyLCPlatform.cpp
  fboss/agent/platforms/wedge/galaxy/GalaxyPort.cpp
  fboss/agent/platforms/wedge/galaxy/oss/GalaxyPlatform.cpp
  fboss/agent/platforms/wedge/galaxy/oss/GalaxyPort.cpp
  fboss/agent/platforms/wedge/oss/WedgePlatform.cpp
  fboss/agent/platforms/wedge/oss/WedgePlatformInit.cpp
  fboss/agent/platforms/wedge/wedge100/Wedge100Platform.cpp
  fboss/agent/platforms/wedge/wedge100/Wedge100Port.cpp
  fboss/agent/platforms/wedge/wedge100/oss/Wedge100Port.cpp
  fboss/agent/platforms/wedge/wedge40/FakeWedge40Platform.cpp
  fboss/agent/platforms/wedge/wedge40/Wedge40Platform.cpp
  fboss/agent/platforms/wedge/wedge40/Wedge40Port.cpp
  fboss/agent/platforms/wedge/wedge40/oss/Wedge40Port.cpp
)

target_link_libraries(platform
  bcm
  product_info
  fboss_config_utils
  qsfp_cpp2
  Folly::folly
  wedge_led_utils
)

add_library(bcm_led_utils
  fboss/agent/platforms/wedge/utils/oss/BcmLedUtils.cpp
)

target_link_libraries(bcm_led_utils
  error
  Folly::folly
)
