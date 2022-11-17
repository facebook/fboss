# CMake to build libraries and binaries in fboss/agent/platforms/common

# In general, libraries and binaries in fboss/foo/bar are built by
# cmake/FooBar.cmake

add_library(platform_mapping
  fboss/agent/platforms/common/MultiPimPlatformMapping.cpp
  fboss/agent/platforms/common/PlatformMapping.cpp
)

target_link_libraries(platform_mapping
  error
  fboss_config_utils
  platform_config_cpp2
  state
  ${RE2}
)

add_library(wedge_led_utils
  fboss/agent/platforms/common/utils/GalaxyLedUtils.cpp
  fboss/agent/platforms/common/utils/Wedge100LedUtils.cpp
  fboss/agent/platforms/common/utils/Wedge40LedUtils.cpp
  fboss/agent/platforms/common/utils/Wedge400LedUtils.cpp
)

target_link_libraries(wedge_led_utils
  error
  ctrl_cpp2
  fboss_types
  transceiver_cpp2
  fboss_i2c_lib
  qsfp_lib
)

add_library(bcm_yaml_config
  fboss/agent/platforms/common/utils/BcmYamlConfig.cpp
)

target_link_libraries(bcm_yaml_config
  platform_mapping
  ${YAML-CPP}
  ${YAML} 
)
