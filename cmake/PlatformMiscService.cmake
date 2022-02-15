# Make to build libraries and binaries in fboss/platform/weutils

# In general, libraries and binaries in fboss/foo/bar are built by
# cmake/FooBar.cmake

add_library(misc_service_lib
  fboss/platform/misc_service/MiscServiceImpl.cpp
  fboss/platform/misc_service/MiscServiceThriftHandler.cpp
  fboss/platform/misc_service/SetupMiscServiceThrift.cpp
  fboss/platform/misc_service/oss/SetupMiscServiceThrift.cpp
)

target_link_libraries(misc_service_lib
  log_thrift_call
  product_info
  platform_utils
  misc_service_cpp2
  Folly::folly
  FBThrift::thriftcpp2
)

add_executable(misc_service
  fboss/platform/misc_service/Main.cpp
)

target_link_libraries(misc_service
  misc_service_lib
  fb303::fb303
)
