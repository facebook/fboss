# Make to build libraries and binaries in fboss/platform/weutils

# In general, libraries and binaries in fboss/foo/bar are built by
# cmake/FooBar.cmake

add_library(weutil_lib
  fboss/platform/weutil/WeutilDarwin.cpp
  fboss/platform/weutil/prefdl/Prefdl.cpp
  fboss/platform/weutil/Weutil.cpp
)

target_link_libraries(weutil_lib
  product_info
  platform_utils
  Folly::folly
)

add_executable(weutil
  fboss/platform/weutil/main.cpp
  fboss/platform/weutil/Flags.cpp
)

target_link_libraries(weutil
  weutil_lib
)

install(TARGETS weutil)
