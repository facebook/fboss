# CMake to build libraries and binaries in fboss/qsfp_service

# In general, libraries and binaries in fboss/foo/bar are built by
# cmake/FooBar.cmake

add_library(qsfp_lib
  fboss/qsfp_service/fsdb/QsfpFsdbSubscriber.cpp
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
    pim_state_cpp2
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
    fsdb_syncer
    fsdb_model
    qsfp_bsp_core
    thrift_cow_serializer
    io_stats_recorder
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
  FBThrift::thriftcpp2
)

add_library(meru400bfu_bsp
  fboss/lib/bsp/meru400bfu/Meru400bfuBspPlatformMapping.cpp
)

target_link_libraries(meru400bfu_bsp
  bsp_platform_mapping_cpp2
  FBThrift::thriftcpp2
)

add_library(meru400bia_bsp
  fboss/lib/bsp/meru400bia/Meru400biaBspPlatformMapping.cpp
)

target_link_libraries(meru400bia_bsp
  bsp_platform_mapping_cpp2
  FBThrift::thriftcpp2
)

add_library(meru400biu_bsp
  fboss/lib/bsp/meru400biu/Meru400biuBspPlatformMapping.cpp
)

target_link_libraries(meru400biu_bsp
  bsp_platform_mapping_cpp2
  FBThrift::thriftcpp2
)

add_library(meru800bia_bsp
  fboss/lib/bsp/meru800bia/Meru800biaBspPlatformMapping.cpp
)

target_link_libraries(meru800bia_bsp
  bsp_platform_mapping_cpp2
  FBThrift::thriftcpp2
)

add_library(meru800bfa_bsp
  fboss/lib/bsp/meru800bfa/Meru800bfaBspPlatformMapping.cpp
)

target_link_libraries(meru800bfa_bsp
  bsp_platform_mapping_cpp2
  FBThrift::thriftcpp2
)

add_library(montblanc_bsp
  fboss/lib/bsp/montblanc/MontblancBspPlatformMapping.cpp
)

target_link_libraries(montblanc_bsp
  bsp_platform_mapping_cpp2
  FBThrift::thriftcpp2
)

add_library(minipack3n_bsp
  fboss/lib/bsp/minipack3n/Minipack3NBspPlatformMapping.cpp
)

target_link_libraries(minipack3n_bsp
  bsp_platform_mapping_cpp2
  FBThrift::thriftcpp2
)

add_library(morgan800cc_bsp
  fboss/lib/bsp/morgan800cc/Morgan800ccBspPlatformMapping.cpp
)

target_link_libraries(morgan800cc_bsp
  bsp_platform_mapping_cpp2
  FBThrift::thriftcpp2
)

add_library(janga800bic_bsp
  fboss/lib/bsp/janga800bic/Janga800bicBspPlatformMapping.cpp
)

target_link_libraries(janga800bic_bsp
  bsp_platform_mapping_cpp2
  FBThrift::thriftcpp2
)

add_library(tahan800bc_bsp
  fboss/lib/bsp/tahan800bc/Tahan800bcBspPlatformMapping.cpp
)

target_link_libraries(tahan800bc_bsp
  bsp_platform_mapping_cpp2
  FBThrift::thriftcpp2
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
  meru400bfu_bsp
  meru400bia_bsp
  meru400biu_bsp
  meru800bia_bsp
  meru800bfa_bsp
  montblanc_bsp
  minipack3n_bsp
  morgan800cc_bsp
  janga800bic_bsp
  tahan800bc_bsp
  device_mdio
  fpga_device
  phy_management_base
  i2c_ctrl
  fpga_multi_pim_container
  ledIO
  led_mapping_cpp2
)

add_library(transceiver_validator
  fboss/qsfp_service/TransceiverValidator.cpp
)

target_link_libraries(transceiver_validator
  transceiver_validation_cpp2
  Folly::folly
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
  fsdb_flags
  firmware_upgrader
  transceiver_validator
  ${RE2}
)

add_library(qsfp_handler
  fboss/qsfp_service/QsfpServiceHandler.cpp
)

target_link_libraries(qsfp_handler
  Folly::folly
  transceiver_manager
  log_thrift_call
  fsdb_stream_client
  fsdb_pub_sub
  fsdb_flags
)

add_library(qsfp_core
  fboss/qsfp_service/QsfpServer.cpp
  fboss/qsfp_service/QsfpServiceSignalHandler.cpp
  fboss/qsfp_service/oss/QsfpServer.cpp
)

target_link_libraries(qsfp_core
  qsfp_handler
)

add_executable(qsfp_service
    fboss/qsfp_service/Main.cpp
)

target_link_libraries(qsfp_service
    qsfp_module
    qsfp_config
    phy_management_base
    transceiver_manager
    qsfp_platforms_wedge
    log_thrift_call
    qsfp_core
    qsfp_handler
)

install(TARGETS qsfp_service)
