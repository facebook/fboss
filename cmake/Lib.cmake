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

add_library(alert_logger
  fboss/lib/AlertLogger.cpp
)

target_link_libraries(alert_logger
  Folly::folly
)

add_library(ref_map
  fboss/lib/RefMap.h
)

set_target_properties(ref_map PROPERTIES LINKER_LANGUAGE CXX)

add_library(tuple_utils
  fboss/lib/TupleUtils.h
)

set_target_properties(tuple_utils PROPERTIES LINKER_LANGUAGE CXX)

add_library(exponential_back_off
  fboss/lib/ExponentialBackoff.cpp
)

target_link_libraries(exponential_back_off
  Folly::folly
)

add_library(function_call_time_reporter
  fboss/lib/FunctionCallTimeReporter.cpp
)

target_link_libraries(function_call_time_reporter
  Folly::folly
)
