# CMake to build libraries and binaries in fboss/agent

# In general, libraries and binaries in fboss/foo/bar are built by
# cmake/FooBar.cmake

add_library(fsdb_oper_metadata_tracker
  fboss/fsdb/server/FsdbOperTreeMetadataTracker.cpp
)

target_link_libraries(fsdb_oper_metadata_tracker
  Folly::folly
  fsdb_common_cpp2
  fsdb_oper_cpp2
)
