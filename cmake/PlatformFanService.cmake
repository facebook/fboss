# Make to build libraries and binaries in fboss/platform/fan_service

# In general, libraries and binaries in fboss/foo/bar are built by
# cmake/FooBar.cmake

add_library(fan_service_lib
  fboss/platform/fan_service/Bsp.cpp
  fboss/platform/fan_service/ControlLogic.cpp
  fboss/platform/fan_service/DarwinFSConfig.cpp
  fboss/platform/fan_service/FanService.cpp
  fboss/platform/fan_service/FanServiceHandler.cpp
  fboss/platform/fan_service/Mokujin.cpp
  fboss/platform/fan_service/MokujinFSConfig.cpp
  fboss/platform/fan_service/SensorData.cpp
  fboss/platform/fan_service/ServiceConfig.cpp
  fboss/platform/fan_service/SetupThrift.cpp
  fboss/platform/fan_service/oss/OssHelper.cpp
  fboss/platform/fan_service/oss/SetupThrift.cpp
)

target_link_libraries(fan_service_lib
  log_thrift_call
  product_info
  platform_utils
  fan_config_structs_types_cpp2
  Folly::folly
  FBThrift::thriftcpp2
  qsfp_cache
)

add_executable(fan_service
  fboss/platform/fan_service/Main.cpp
)

target_link_libraries(fan_service
  fan_service_lib
  fb303::fb303
)
