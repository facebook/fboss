# CMake to build libraries and binaries in fboss/qsfp_service

# In general, libraries and binaries in fboss/foo/bar are built by
# cmake/FooBar.cmake

add_library(qsfp_lib
  fboss/qsfp_service/fsdb/QsfpFsdbSyncManager.cpp
  fboss/qsfp_service/fsdb/oss/QsfpFsdbSyncManager.cpp
  fboss/qsfp_service/oss/StatsPublisher.cpp
  fboss/qsfp_service/platforms/wedge/WedgeI2CBusLock.cpp
  fboss/qsfp_service/platforms/wedge/WedgeQsfp.cpp
  fboss/qsfp_service/lib/QsfpCache.cpp
)

target_link_libraries(qsfp_lib
    qsfp_cpp2
    ctrl_cpp2
    i2c_controller_stats_cpp2
    transceiver_cpp2
    alert_logger
    Folly::folly
    fb303::fb303
    FBThrift::thriftcpp2
    qsfp_service_client
    fsdb_stream_client
    fsdb_pub_sub
    fsdb_flags
    qsfp_bsp_core
)

add_library(qsfp_config
  fboss/qsfp_service/QsfpConfig.cpp
)

target_link_libraries(qsfp_config
  error
  qsfp_config_cpp2
  Folly::folly
  FBThrift::thriftcpp2
)

add_library(bsp_platform_mapping
  fboss/lib/bsp/BspPlatformMapping.cpp
)

target_link_libraries(bsp_platform_mapping
  bsp_platform_mapping_cpp2
)

add_library(kamet_bsp
  fboss/lib/bsp/kamet/KametBspPlatformMapping.cpp
)

target_link_libraries(kamet_bsp
  bsp_platform_mapping_cpp2
)

add_library(makalu_bsp
  fboss/lib/bsp/makalu/MakaluBspPlatformMapping.cpp
)

target_link_libraries(makalu_bsp
  bsp_platform_mapping_cpp2
)

add_library(qsfp_bsp_core
  fboss/lib/bsp/BspGenericSystemContainer.cpp
  fboss/lib/bsp/BspIOBus.cpp
  fboss/lib/bsp/BspPimContainer.cpp
  fboss/lib/bsp/BspSystemContainer.cpp
  fboss/lib/bsp/BspPhyContainer.cpp
  fboss/lib/bsp/BspPhyIO.cpp
  fboss/lib/bsp/BspTransceiverAccess.cpp
  fboss/lib/bsp/BspTransceiverAccessImpl.cpp
  fboss/lib/bsp/BspTransceiverCpldAccess.cpp
  fboss/lib/bsp/BspTransceiverApi.cpp
  fboss/lib/bsp/BspTransceiverContainer.cpp
  fboss/lib/bsp/BspTransceiverIO.cpp
  fboss/lib/bsp/BspLedContainer.cpp
)

target_link_libraries(qsfp_bsp_core
  bsp_platform_mapping_cpp2
  bsp_platform_mapping
  common_file_utils
  i2c_controller_stats_cpp2
  Folly::folly
  kamet_bsp
  makalu_bsp
  device_mdio
  fpga_device
  phy_management_base
  i2c_ctrl
  fpga_multi_pim_container
  ledIO
  led_mapping_cpp2
)

add_library(transceiver_manager STATIC
    fboss/qsfp_service/TransceiverManager.cpp
    fboss/qsfp_service/TransceiverStateMachine.cpp
    fboss/qsfp_service/TransceiverStateMachineUpdate.cpp
)

target_link_libraries(transceiver_manager
  fboss_error
  fboss_types
  qsfp_config
  phy_management_base
  thrift_service_client
  common_file_utils
  qsfp_platforms_wedge
  thread_heartbeat
  utils
  product_info
)
