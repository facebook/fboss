# CMake to build libraries and binaries in fboss/agent

# In general, libraries and binaries in fboss/foo/bar are built by
# cmake/FooBar.cmake

add_library(fsdb_flags
  fboss/fsdb/Flags.cpp
)

target_link_libraries(fsdb_flags
  fsdb_common_cpp2
  FBThrift::thriftcpp2
)

