# Make to build libraries and binaries in fboss/platform/weutils

# In general, libraries and binaries in fboss/foo/bar are built by
# cmake/FooBar.cmake

add_fbthrift_cpp_library(
  fw_util_config-cpp2-types
  fboss/platform/fw_util/if/fw_util_config.thrift
  OPTIONS
    json
    reflection
)

add_executable(fw_util
  fboss/platform/fw_util/fw_util.cpp
  fboss/platform/fw_util/Flags.cpp
  fboss/platform/fw_util/FwUtilImpl.cpp
)

target_link_libraries(fw_util
  Folly::folly
  platform_config_lib
  platform_name_lib
  platform_utils
  FBThrift::thriftcpp2
  fw_util_config-cpp2-types
)

install(TARGETS fw_util)
