# Make to build libraries and binaries in fboss/platform/weutils

# In general, libraries and binaries in fboss/foo/bar are built by
# cmake/FooBar.cmake

add_fbthrift_cpp_library(
  sensor_service_cpp2
  fboss/platform/sensor_service/if/sensor_service.thrift
  SERVICES
    SensorServiceThrift
  OPTIONS
    json
    reflection
  DEPENDS
    fboss_cpp2
)

add_fbthrift_cpp_library(
  sensor_config_cpp2
  fboss/platform/sensor_service/if/sensor_config.thrift
  OPTIONS
    json
    reflection
)

add_library(sensor_service_lib
  fboss/platform/sensor_service/FsdbSyncer.cpp
  fboss/platform/sensor_service/Flags.cpp
  fboss/platform/sensor_service/SensorServiceImpl.cpp
  fboss/platform/sensor_service/SensorServiceThriftHandler.cpp
  fboss/platform/sensor_service/oss/FsdbSyncer.cpp
  fboss/platform/sensor_service/oss/SensorStatsPub.cpp
)

target_link_libraries(sensor_service_lib
  log_thrift_call
  platform_config_lib
  platform_utils
  sensor_service_cpp2
  sensor_config_cpp2
  Folly::folly
  FBThrift::thriftcpp2
  fsdb_stream_client
  fsdb_pub_sub
  fsdb_flags
)

add_executable(sensor_service
  fboss/platform/sensor_service/Main.cpp
)

target_link_libraries(sensor_service
  sensor_service_lib
  fb303::fb303
)

add_executable(sensor_service_hw_test
  fboss/platform/sensor_service/hw_test/Main.cpp
  fboss/platform/sensor_service/hw_test/SensorsTest.cpp
)

target_link_libraries(sensor_service_hw_test
  sensor_service_lib
  ${GTEST}
  ${LIBGMOCK_LIBRARIES}
)

add_executable(sensor_service_impl_test
  fboss/platform/sensor_service/test/SensorServiceImplTest.cpp
  fboss/platform/sensor_service/test/TestUtils.cpp
)

target_link_libraries(sensor_service_impl_test
  platform_utils
  sensor_service_lib
  ${GTEST}
  ${LIBGMOCK_LIBRARIES}
)
