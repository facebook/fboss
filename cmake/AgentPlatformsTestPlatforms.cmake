# CMake to build libraries and binaries in fboss/agent/platforms/test_platforms

# In general, libraries and binaries in fboss/foo/bar are built by
# cmake/FooBar.cmake

add_library(bcm_test_platforms
  fboss/agent/platforms/tests/utils/BcmTestPort.cpp
  fboss/agent/platforms/tests/utils/BcmTestPlatform.cpp
  fboss/agent/platforms/tests/utils/BcmTestWedgePlatform.cpp
  fboss/agent/platforms/tests/utils/BcmTestWedge40Platform.cpp
  fboss/agent/platforms/tests/utils/BcmTestWedge40Port.cpp
  fboss/agent/platforms/tests/utils/BcmTestWedge100Platform.cpp
  fboss/agent/platforms/tests/utils/BcmTestWedge100Port.cpp
  fboss/agent/platforms/tests/utils/BcmTestGalaxyPlatform.cpp
  fboss/agent/platforms/tests/utils/BcmTestGalaxyPort.cpp
  fboss/agent/platforms/tests/utils/BcmTestWedge400Platform.cpp
  fboss/agent/platforms/tests/utils/BcmTestWedge400Port.cpp
  fboss/agent/platforms/tests/utils/BcmTestMinipackPlatform.cpp
  fboss/agent/platforms/tests/utils/BcmTestMinipackPort.cpp
  fboss/agent/platforms/tests/utils/BcmTestYampPlatform.cpp
  fboss/agent/platforms/tests/utils/BcmTestYampPort.cpp
  fboss/agent/platforms/tests/utils/FakeBcmTestPlatform.cpp
  fboss/agent/platforms/tests/utils/FakeBcmTestPort.cpp
  fboss/agent/platforms/tests/utils/CreateTestPlatform.cpp
  fboss/agent/platforms/tests/utils/BcmTestWedgeTomahawkPlatform.cpp
  fboss/agent/platforms/tests/utils/BcmTestWedgeTomahawk3Platform.cpp
  fboss/agent/platforms/tests/utils/BcmTestWedge40Platform.cpp
  fboss/agent/platforms/tests/utils/FakeBcmTestPlatform.cpp
  fboss/agent/platforms/tests/utils/BcmTestTomahawk4Platform.cpp
  fboss/agent/platforms/tests/utils/BcmTestFujiPlatform.cpp
  fboss/agent/platforms/tests/utils/BcmTestFujiPort.cpp
  fboss/agent/platforms/tests/utils/BcmTestElbertPlatform.cpp
  fboss/agent/platforms/tests/utils/BcmTestElbertPort.cpp
  fboss/agent/platforms/tests/utils/BcmTestDarwinPlatform.cpp
  fboss/agent/platforms/tests/utils/BcmTestDarwinPort.cpp
)

target_link_libraries(bcm_test_platforms
  bcm
  bcm_led_utils
  config_factory
  product_info
  fake_test_platform_mapping
  galaxy_platform_mapping
  minipack_platform_mapping
  wedge100_platform_mapping
  wedge40_platform_mapping
  wedge400_platform_mapping
  yamp_platform_mapping
  fuji_platform_mapping
  elbert_platform_mapping
  darwin_platform_mapping
  meru400biu_platform_mapping
  meru400bia_platform_mapping
  kamet_platform_mapping
  ${GTEST}
  ${LIBGMOCK_LIBRARIES}
)
