# CMake to build libraries and binaries in fboss/agent

# In general, libraries and binaries in fboss/foo/bar are built by
# cmake/FooBar.cmake

add_library(fsdb_flags
  fboss/fsdb/common/Flags.cpp
)

target_link_libraries(fsdb_flags
  fsdb_common_cpp2
  FBThrift::thriftcpp2
)

add_library(fsdb_utils
  fboss/fsdb/common/Utils.cpp
)

target_link_libraries(fsdb_utils
  Folly::folly
  fsdb_common_cpp2
  fsdb_oper_cpp2
  FBThrift::thriftcpp2
  cow_storage
)
