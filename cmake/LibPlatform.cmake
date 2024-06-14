# CMake to build libraries and binaries in fboss/lib/platforms/

# In general, libraries and binaries in fboss/foo/bar are built by
# cmake/FooBar.cmake

add_library(product_info
  fboss/lib/platforms/PlatformProductInfo.cpp
  fboss/lib/platforms/oss/PlatformProductInfo.cpp
)

target_link_libraries(product_info
  product_info_cpp2
  Folly::folly
  fboss_error
)

add_library(platform_mode
  fboss/lib/platforms/PlatformMode.h
)

target_link_libraries(platform_mode
  fboss_common_cpp2
)
