# Make to build libraries and binaries in fboss/platform/weutils

# In general, libraries and binaries in fboss/foo/bar are built by
# cmake/FooBar.cmake

add_fbthrift_cpp_library(
  data_corral_service_cpp2
  fboss/platform/data_corral_service/if/data_corral_service.thrift
  SERVICES
    DataCorralServiceThrift
  OPTIONS
    json
    reflection
  DEPENDS
    fb303_cpp2
    fboss_cpp2
)

add_fbthrift_cpp_library(
  led_manager_config_types_cpp2
  fboss/platform/data_corral_service/if/led_manager_config.thrift
  OPTIONS
    json
    reflection
)

add_library(data_corral_service_lib
  fboss/platform/data_corral_service/DataCorralServiceThriftHandler.cpp
  fboss/platform/data_corral_service/FruPresenceExplorer.cpp
  fboss/platform/data_corral_service/LedManager.cpp
  fboss/platform/data_corral_service/ConfigValidator.cpp
)

target_link_libraries(data_corral_service_lib
  log_thrift_call
  product_info
  common_file_utils
  weutil_lib
  platform_utils
  platform_name_lib
  data_corral_service_cpp2
  led_manager_config_types_cpp2
  Folly::folly
  fb303::fb303
  FBThrift::thriftcpp2
  platform_manager_utils
)

add_executable(data_corral_service
  fboss/platform/data_corral_service/Main.cpp
)

target_link_libraries(data_corral_service
  data_corral_service_lib
)

add_executable(data_corral_service_hw_test
  fboss/platform/data_corral_service/hw_test/DataCorralServiceHwTest.cpp
)

target_link_libraries(data_corral_service_hw_test
  data_corral_service_lib
  ${GTEST}
  ${LIBGMOCK_LIBRARIES}
)
