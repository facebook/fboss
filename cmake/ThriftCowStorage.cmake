# CMake to build libraries and binaries in fboss/agent/capture

# In general, libraries and binaries in fboss/foo/bar are built by
# cmake/FooBar.cmake

add_library(
  storage
  fboss/thrift_cow/storage/Storage.h
)

set_target_properties(storage PROPERTIES LINKER_LANGUAGE CXX)

target_link_libraries(storage
  fsdb_oper_cpp2
  patch_cpp2
  Folly::folly
)


add_library(
  cow_storage
  fboss/thrift_cow/storage/CowStorage.h
  fboss/thrift_cow/storage/CowStorage.cpp
)

set_target_properties(cow_storage PROPERTIES LINKER_LANGUAGE CXX)

target_link_libraries(cow_storage
  storage
  thrift_cow_nodes
  thrift_cow_visitors
  Folly::folly
)


add_library(
  cow_storage_mgr
  fboss/thrift_cow/storage/CowStateUpdate.h
  fboss/thrift_cow/storage/CowStorageMgr.h
)

set_target_properties(cow_storage_mgr PROPERTIES LINKER_LANGUAGE CXX)

target_link_libraries(cow_storage_mgr
  cow_storage
  Folly::folly
)
