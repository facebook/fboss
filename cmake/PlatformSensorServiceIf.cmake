# CMake to build libraries and binaries in fboss/agent

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
