# Make to build libraries and binaries in fboss/platform/weutils

# In general, libraries and binaries in fboss/foo/bar are built by
# cmake/FooBar.cmake

add_library(rackmon_lib
  fboss/platform/rackmon/RackmonThriftHandler.cpp
)

target_link_libraries(rackmon_lib
  rackmon_cpp2
  Folly::folly
)

add_executable(rackmon
  fboss/platform/rackmon/Main.cpp
)

target_link_libraries(rackmon
  platform_utils
  rackmon_lib
  fb303::fb303
)
