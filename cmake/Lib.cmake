# CMake to build libraries and binaries in fboss/lib

# In general, libraries and binaries in fboss/foo/bar are built by
# cmake/FooBar.cmake

add_library(radix_tree
  fboss/lib/RadixTree.h
  fboss/lib/RadixTree-inl.h
)

target_link_libraries(radix_tree
  Folly::folly
)

add_library(log_thrift_call
  fboss/lib/LogThriftCall.cpp
)

target_link_libraries(log_thrift_call
  Folly::folly
  FBThrift::thriftcpp2
)

add_library(exponential_back_off
  fboss/lib/ExponentialBackoff.cpp
)

target_link_libraries(exponential_back_off
  Folly::folly
)
