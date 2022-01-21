# Make to build libraries and binaries in fboss/platform/weutils

# In general, libraries and binaries in fboss/foo/bar are built by
# cmake/FooBar.cmake

add_executable(weutil
  fboss/platform/weutil/WeutilDarwin.cpp
  fboss/platform/weutil/main.cpp
  fboss/platform/weutil/prefdl/Prefdl.cpp
)

target_link_libraries(weutil
  product_info
  platform_utils
  Folly::folly
)

