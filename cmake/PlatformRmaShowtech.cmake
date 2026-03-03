# Make to build libraries and binaries in fboss/platform/rma-showtech

# In general, libraries and binaries in fboss/foo/bar are built by
# cmake/FooBar.cmake

add_fbthrift_cpp_library(
  showtech_config_cpp2
  fboss/platform/rma-showtech/showtech_config.thrift
  OPTIONS
    json
    reflection
)

add_executable(rma-showtech
  fboss/platform/rma-showtech/FanImpl.cpp
  fboss/platform/rma-showtech/I2cHelper.cpp
  fboss/platform/rma-showtech/Main.cpp
  fboss/platform/rma-showtech/PsuHelper.cpp
  fboss/platform/rma-showtech/Utils.cpp
)

target_link_libraries(rma-showtech
  fmt::fmt
  fb303::fb303
  platform_utils
  showtech_config_cpp2
  platform_config_lib
  platform_name_lib
  common_file_utils
  i2c_ctrl
  gpiod_line
  fan_service_config_types_cpp2
  ${LIBGPIOD}
  ${RE2}
  CLI11::CLI11
)

install(TARGETS rma-showtech)
