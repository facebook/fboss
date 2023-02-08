# Make to build libraries and binaries in fboss/platform/fbdevd

# In general, libraries and binaries in fboss/foo/bar are built by
# cmake/FooBar.cmake

add_fbthrift_cpp_library(
  fbdevd_cpp2
  fboss/platform/fbdevd/if/fbdevd.thrift
  SERVICES
    FbdevManager
  OPTIONS
    json
    reflection
)

add_fbthrift_cpp_library(
  gpio_cpp2
  fboss/platform/fbdevd/if/gpio.thrift
  OPTIONS
    json
    reflection
)

add_fbthrift_cpp_library(
  i2c_cpp2
  fboss/platform/fbdevd/if/i2c.thrift
  OPTIONS
    json
    reflection
)


add_library(fbdevd_lib
  fboss/platform/fbdevd/FbdevdImpl.cpp
  fboss/platform/fbdevd/Flags.cpp
  fboss/platform/fbdevd/FruManager.cpp
  fboss/platform/fbdevd/I2cController.cpp
)

target_link_libraries(fbdevd_lib
  log_thrift_call
  common_file_utils
  platform_utils
  platform_config_lib
  fbdevd_cpp2
  gpio_cpp2
  i2c_cpp2
  Folly::folly
  FBThrift::thriftcpp2
)

add_executable(fbdevd
  fboss/platform/fbdevd/Main.cpp
)

target_link_libraries(fbdevd
  fbdevd_lib
  fb303::fb303
)
