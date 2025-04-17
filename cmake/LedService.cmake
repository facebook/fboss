# Make to build libraries and binaries in fboss/led_service

# In general, libraries and binaries in fboss/foo/bar are built by
# cmake/FooBar.cmake

add_fbthrift_cpp_library(
  led_structs_types_cpp2
  fboss/led_service/if/led_structs.thrift
  OPTIONS
    json
    reflection
)

add_fbthrift_cpp_library(
  led_service_types_cpp2
  fboss/led_service/if/led_service.thrift
  SERVICES
    LedService
  OPTIONS
    json
    reflection
  DEPENDS
    fboss_cpp2
    ctrl_cpp2
    led_structs_types_cpp2
)

add_library(led_utils
  fboss/led_service/LedUtils.cpp
)

target_link_libraries(led_utils
  led_structs_types_cpp2
)

add_library(led_config
  fboss/led_service/LedConfig.cpp
)

target_link_libraries(led_config
  error
  led_config_cpp2
  Folly::folly
  FBThrift::thriftcpp2
)

add_library(led_manager_lib
  fboss/led_service/BspLedManager.cpp
  fboss/led_service/FsdbSwitchStateSubscriber.cpp
  fboss/led_service/oss/DarwinLedManager.cpp
  fboss/led_service/oss/ElbertLedManager.cpp
  fboss/led_service/oss/FsdbSwitchStateSubscriber.cpp
  fboss/led_service/oss/FujiLedManager.cpp
  fboss/led_service/oss/MinipackLedManager.cpp
  fboss/led_service/oss/Wedge400BaseLedManager.cpp
  fboss/led_service/oss/Wedge400CLedManager.cpp
  fboss/led_service/oss/Wedge400LedManager.cpp
  fboss/led_service/oss/YampLedManager.cpp
  fboss/led_service/LedManager.cpp
  fboss/led_service/LedManagerInit.cpp
  fboss/led_service/LedUtils.cpp
  fboss/led_service/MinipackBaseLedManager.cpp
  fboss/led_service/MontblancLedManager.cpp
  fboss/led_service/Meru800biaLedManager.cpp
  fboss/led_service/Meru800bfaLedManager.cpp
  fboss/led_service/Morgan800ccLedManager.cpp
  fboss/led_service/Minipack3NLedManager.cpp
)

target_link_libraries(led_manager_lib
  bsp_platform_mapping_cpp2
  qsfp_bsp_core
  ledIO
  led_config
  led_structs_types_cpp2
  log_thrift_call
  led_utils
  montblanc_bsp
  meru800bia_bsp
  meru800bfa_bsp
  morgan800cc_bsp
  darwin_platform_mapping
  elbert_platform_mapping
  fuji_platform_mapping
  minipack_platform_mapping
  minipack3n_platform_mapping
  montblanc_platform_mapping
  meru800bia_platform_mapping
  meru800bfa_platform_mapping
  morgan_platform_mapping
  wedge400_platform_mapping
  yamp_platform_mapping
  product_info
  Folly::folly
  FBThrift::thriftcpp2
  fsdb_stream_client
  fsdb_pub_sub
  fsdb_flags
)

add_library(led_core_lib
  fboss/led_service/LedService.cpp
  fboss/led_service/LedServiceHandler.cpp
)

target_link_libraries(led_core_lib
  led_manager_lib
  led_service_types_cpp2
)

add_executable(led_service
  fboss/led_service/Main.cpp
)

target_link_libraries(led_service
  led_core_lib
  platform_utils
  fb303::fb303
)

install(TARGETS led_service)
