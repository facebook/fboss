# CMake to build libraries and binaries in fboss/qsfp_service/module

# In general, libraries and binaries in fboss/foo/bar are built by
# cmake/FooBar.cmake

add_library(firmware_upgrader
  fboss/qsfp_service/module/CdbCommandBlock.cpp
  fboss/qsfp_service/module/FirmwareUpgrader.cpp
  fboss/qsfp_service/module/CdbCommandBlock.h
  fboss/qsfp_service/module/FirmwareUpgrader.h
  fboss/qsfp_service/module/Transceiver.h
  fboss/qsfp_service/module/TransceiverImpl.h
)

target_link_libraries(firmware_upgrader
  fboss_error
  fboss_types
  switch_config_cpp2
  ctrl_cpp2
  firmware_storage
  phy_cpp2
  prbs_cpp2
  transceiver_access_parameter
  qsfp_config_cpp2
  port_state_cpp2
  transceiver_cpp2
  cmis_cpp2
  Folly::folly
)

add_library(qsfp_module
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
  fboss/qsfp_service/module/cmis/oss/CmisModule.cpp
)

target_link_libraries(qsfp_module
  firmware_upgrader
  fboss_error
  fboss_types
  switch_config_cpp2
  firmware_storage
  async_filewriter_factory
  snapshot_manager
  phy_cpp2
  prbs_cpp2
  i2_api
  qsfp_stats
  transceiver_manager
  qsfp_config_cpp2
  transceiver_cpp2
  qsfp_config_parser
  qsfp_service_client
  cmis_cpp2
  sff_cpp2
  sff8472_cpp2
  Folly::folly
)

add_library(i2c_log_buffer
  fboss/qsfp_service/module/I2cLogBuffer.cpp
  fboss/qsfp_service/module/I2cLogBuffer.h
)

target_link_libraries(i2c_log_buffer
  transceiver_access_parameter
  qsfp_config_cpp2
  cmis_cpp2
  sff_cpp2
  sff8472_cpp2
  Folly::folly
)
