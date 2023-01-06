# Make to build libraries and binaries in fboss/platform/weutils

# In general, libraries and binaries in fboss/foo/bar are built by
# cmake/FooBar.cmake

add_library(data_corral_service_lib
  fboss/platform/data_corral_service/DataCorralServiceImpl.cpp
  fboss/platform/data_corral_service/DataCorralServiceThriftHandler.cpp
  fboss/platform/data_corral_service/Flags.cpp
  fboss/platform/data_corral_service/SetupDataCorralServiceThrift.cpp
  fboss/platform/data_corral_service/oss/SetupDataCorralServiceThrift.cpp
  fboss/platform/data_corral_service/ChassisManager.cpp
  fboss/platform/data_corral_service/oss/ChassisManager.cpp
  fboss/platform/data_corral_service/darwin/DarwinChassisManager.cpp
  fboss/platform/data_corral_service/darwin/DarwinPlatformConfig.cpp
  fboss/platform/data_corral_service/darwin/DarwinFruModule.cpp
)

target_link_libraries(data_corral_service_lib
  log_thrift_call
  product_info
  common_file_utils
  weutil_lib
  platform_utils
  data_corral_service_cpp2
  Folly::folly
  FBThrift::thriftcpp2
)

add_executable(data_corral_service
  fboss/platform/data_corral_service/Main.cpp
)

target_link_libraries(data_corral_service
  data_corral_service_lib
  fb303::fb303
)
