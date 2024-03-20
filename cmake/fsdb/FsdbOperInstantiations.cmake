# CMake to build libraries and binaries in fboss/agent

# In general, libraries and binaries in fboss/foo/bar are built by
# cmake/FooBar.cmake

add_library(fsdb_cow_root
  fboss/fsdb/oper/instantiations/FsdbCowRoot.cpp
)

target_link_libraries(fsdb_cow_root
  fsdb_model
  thrift_cow_nodes
)

add_library(fsdb_cow_storage
  fboss/fsdb/oper/instantiations/FsdbCowStorage.cpp
)

target_link_libraries(fsdb_cow_storage
  fsdb_model
  cow_storage
  fsdb_cow_root
)

add_library(fsdb_cow_subscription_manager
  fboss/fsdb/oper/instantiations/FsdbCowSubscriptionManager.cpp
)

target_link_libraries(fsdb_cow_subscription_manager
  fsdb_model
  cow_storage
  fsdb_cow_storage
  subscription_manager
)

add_library(fsdb_naive_periodic_subscribable_storage
  fboss/fsdb/oper/instantiations/FsdbNaivePeriodicSubscribableStorage.cpp
)

target_link_libraries(fsdb_naive_periodic_subscribable_storage
  fsdb_model
  fsdb_cow_storage
  fsdb_cow_subscription_manager
  subscribable_storage
)
