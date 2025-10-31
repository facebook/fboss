# CMake to build libraries and binaries in fboss/qsfp_service/module

# In general, libraries and binaries in fboss/foo/bar are built by
# cmake/FooBar.cmake

add_library(firmware_upgrader
  fboss/qsfp_service/module/CdbCommandBlock.cpp
  fboss/qsfp_service/module/FirmwareUpgrader.cpp
)

target_link_libraries(firmware_upgrader
  cmis_cpp2
  Folly::folly
  transceiver_cpp2
  firmware_storage
  fboss_i2c_lib
  sff_cpp2
  sff8472_cpp2
)

add_library(qsfp_module STATIC
  fboss/qsfp_service/module/I2cLogBuffer.cpp
  fboss/qsfp_service/module/QsfpHelper.cpp
  fboss/qsfp_service/module/QsfpModule.cpp
  fboss/qsfp_service/module/oss/QsfpModule.cpp
  fboss/qsfp_service/module/sff/SffFieldInfo.cpp
  fboss/qsfp_service/module/sff/SffModule.cpp
  fboss/qsfp_service/module/sff/Sff8472Module.cpp
  fboss/qsfp_service/module/sff/Sff8472FieldInfo.cpp
  fboss/qsfp_service/module/oss/SffModule.cpp
  fboss/qsfp_service/module/cmis/CmisFieldInfo.cpp
  fboss/qsfp_service/module/cmis/CmisModule.cpp
)

target_link_libraries(qsfp_module
  fboss_i2c_lib
  qsfp_config_parser
  snapshot_manager
  qsfp_lib
)
