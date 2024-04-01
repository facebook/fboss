# CMake to build libraries and binaries in fboss/agent

# In general, libraries and binaries in fboss/foo/bar are built by
# cmake/FooBar.cmake

if (FBOSS_CENTOS9)

add_library(subscription_manager
  fboss/fsdb/oper/DeltaValue.h
  fboss/fsdb/oper/CowDeletePathTraverseHelper.h
  fboss/fsdb/oper/CowPublishAndAddTraverseHelper.h
  fboss/fsdb/oper/CowSubscriptionManager.h
  fboss/fsdb/oper/CowSubscriptionTraverseHelper.h
  fboss/fsdb/oper/Subscription.cpp
  fboss/fsdb/oper/Subscription.h
  fboss/fsdb/oper/SubscriptionManager.h
  fboss/fsdb/oper/SubscriptionMetadataServer.cpp
  fboss/fsdb/oper/SubscriptionMetadataServer.h
  fboss/fsdb/oper/SubscriptionPathStore.cpp
  fboss/fsdb/oper/SubscriptionPathStore.h
)

target_link_libraries(subscription_manager
  cow_storage
  Folly::folly
  fsdb_common_cpp2
  fsdb_cpp2
  fsdb_oper_cpp2
  fsdb_model
  thrift_cow_visitors
  fsdb_utils
  fsdb_oper_metadata_tracker
)

add_library(path_helpers
  fboss/fsdb/oper/PathValidator.cpp
  fboss/fsdb/oper/PathConverter.h
  fboss/fsdb/oper/PathValidator.h
)

target_link_libraries(path_helpers
  fsdb_config_cpp2
  fsdb_common_cpp2
  fsdb_utils
  fsdb_oper_cpp2
  fsdb_model
  thrift_visitors
  ${RE2}
)

add_library(subscribable_storage
  fboss/fsdb/oper/NaivePeriodicSubscribableStorage.h
  fboss/fsdb/oper/NaivePeriodicSubscribableStorage.cpp
  fboss/fsdb/oper/SubscribableStorage.h
)

target_link_libraries(subscribable_storage
  cow_storage
  path_helpers
  fsdb_config_cpp2
  fsdb_common_cpp2
  fsdb_oper_cpp2
  fsdb_model
  subscription_manager
  thread_heartbeat
  thrift_visitors
  Folly::folly
  fb303::fb303
)
endif()
