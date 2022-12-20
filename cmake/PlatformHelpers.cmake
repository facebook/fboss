# Make to build libraries and binaries in fboss/platform/helpers

# In general, libraries and binaries in fboss/foo/bar are built by
# cmake/FooBar.cmake

add_library(platform_utils
  fboss/platform/helpers/Utils.cpp
  fboss/platform/helpers/oss/Init.cpp
  fboss/platform/helpers/oss/Utils.cpp
)

target_link_libraries(platform_utils
  Folly::folly
  FBThrift::thriftcpp2
  ${RE2}
)
