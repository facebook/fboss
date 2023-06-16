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

add_library(led_core_lib
  fboss/led_service/BspLedManager.cpp
  fboss/led_service/FsdbSwitchStateSubscriber.cpp
  fboss/led_service/oss/FsdbSwitchStateSubscriber.cpp
  fboss/led_service/oss/FujiLedManager.cpp
  fboss/led_service/LedManager.cpp
  fboss/led_service/LedService.cpp
  fboss/led_service/LedServiceHandler.cpp
  fboss/led_service/MinipackBaseLedManager.cpp
  fboss/led_service/MontblancLedManager.cpp
)

target_link_libraries(led_core_lib
  bsp_platform_mapping_cpp2
  qsfp_bsp_core
  ledIO
  led_structs_types_cpp2
  log_thrift_call
  montblanc_bsp
  fuji_platform_mapping
  montblanc_platform_mapping
  product_info
  Folly::folly
  FBThrift::thriftcpp2
  fsdb_stream_client
  fsdb_pub_sub
  fsdb_flags
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
