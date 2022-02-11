
add_fbthrift_cpp_library(
  sensor_service_cpp2
  fboss/platform/sensor_service/if/sensor_service.thrift
  SERVICES
    SensorServiceThrift
  OPTIONS
    json
  DEPENDS
    fboss_cpp2
    ctrl_cpp2
)

add_fbthrift_cpp_library(
  sensor_config_cpp2
  fboss/platform/sensor_service/if/sensor_config.thrift
  OPTIONS
    json
)
