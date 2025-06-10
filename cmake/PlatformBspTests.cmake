
add_fbthrift_cpp_library(
  bsp_tests_config_cpp2
  fboss/platform/bsp_tests/bsp_tests_config.thrift
  OPTIONS
    json
    reflection
  DEPENDS
    fbiob_device_config_cpp2
    platform_manager_config_cpp2

)

add_fbthrift_cpp_library(
  fbiob_device_config_cpp2
  fboss/platform/bsp_tests/fbiob_device_config.thrift
  OPTIONS
    json
    reflection
)
