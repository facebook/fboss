# Make to build libraries and binaries in fboss/platform/fan_service

# In general, libraries and binaries in fboss/foo/bar are built by
# cmake/FooBar.cmake

add_fbthrift_cpp_library(
  fan_service_config_types_cpp2
  fboss/platform/fan_service/if/fan_service_config.thrift
  OPTIONS
    json
    reflection
)

add_fbthrift_cpp_library(
  fan_service_cpp2
  fboss/platform/fan_service/if/fan_service.thrift
  SERVICES
    FanService
  DEPENDS
    fboss_cpp2
)


add_library(fan_service_lib
  fboss/platform/fan_service/Bsp.cpp
  fboss/platform/fan_service/ControlLogic.cpp
  fboss/platform/fan_service/FanServiceHandler.cpp
  fboss/platform/fan_service/FsdbSensorSubscriber.cpp
  fboss/platform/fan_service/PidLogic.cpp
  fboss/platform/fan_service/SensorData.cpp
  fboss/platform/fan_service/Utils.cpp
  fboss/platform/fan_service/oss/FsdbSensorSubscriber.cpp
  fboss/platform/fan_service/oss/DataFetcher.cpp
)

target_link_libraries(fan_service_lib
  log_thrift_call
  product_info
  common_file_utils
  platform_config_lib
  platform_name_lib
  platform_utils
  fan_service_config_types_cpp2
  gpiod_line
  sensor_service_cpp2
  fan_service_cpp2
  Folly::folly
  qsfp_service_client
  qsfp_state_cpp2
  qsfp_stats_cpp2
  FBThrift::thriftcpp2
  fsdb_stream_client
  fsdb_pub_sub
  fsdb_flags
)

add_executable(fan_service
  fboss/platform/fan_service/Main.cpp
)

target_link_libraries(fan_service
  fan_service_lib
  fb303::fb303
)

install(TARGETS fan_service)

add_executable(fan_service_sw_test
  fboss/platform/fan_service/tests/BspTests.cpp
  fboss/platform/fan_service/tests/ControlLogicTests.cpp
)

target_link_libraries(fan_service_sw_test
  fan_service_lib
  Folly::folly
  ${LIBGPIOD}
  gpiod_line
  ${GTEST}
  ${LIBGMOCK_LIBRARIES}
)

install(TARGETS fan_service_sw_test)

add_executable(fan_service_hw_test
  fboss/platform/fan_service/hw_test/FanServiceHwTest.cpp
)

target_link_libraries(fan_service_hw_test
  fan_service_lib
  Folly::folly
  ${GTEST}
  ${LIBGMOCK_LIBRARIES}
)

install(TARGETS fan_service_hw_test)
