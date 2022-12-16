# Make to build libraries and binaries in fboss/platform/weutils

# In general, libraries and binaries in fboss/foo/bar are built by
# cmake/FooBar.cmake

add_library(sensor_service_lib
  fboss/platform/sensor_service/GetSensorConfig.cpp
  fboss/platform/sensor_service/FsdbSyncer.cpp
  fboss/platform/sensor_service/Flags.cpp
  fboss/platform/sensor_service/MockSensorConfig.cpp
  fboss/platform/sensor_service/SensorServiceImpl.cpp
  fboss/platform/sensor_service/DarwinSensorConfig.cpp
  fboss/platform/sensor_service/SensorServiceThriftHandler.cpp
  fboss/platform/sensor_service/oss/FsdbSyncer.cpp
  fboss/platform/sensor_service/oss/SensorStatsPub.cpp
)

target_link_libraries(sensor_service_lib
  log_thrift_call
  product_info
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
