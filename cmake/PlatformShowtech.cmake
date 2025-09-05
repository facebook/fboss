# Make to build libraries and binaries in fboss/platform/showtech

# In general, libraries and binaries in fboss/foo/bar are built by
# cmake/FooBar.cmake

add_fbthrift_cpp_library(
  showtech_config_cpp2
  fboss/platform/showtech/showtech_config.thrift
  OPTIONS
    json
    reflection
)

add_executable(showtech
  fboss/platform/showtech/I2cHelper.cpp
  fboss/platform/showtech/Main.cpp
  fboss/platform/showtech/PsuHelper.cpp
  fboss/platform/showtech/Utils.cpp
)

target_link_libraries(showtech
  fmt::fmt
  fb303::fb303
  platform_utils
  showtech_config_cpp2
  platform_config_lib
  platform_name_lib
  i2c_ctrl
)

install(TARGETS showtech)
