# CMake to build libraries and binaries in fboss/cli/fboss2

# In general, libraries and binaries in fboss/foo/bar are built by
# cmake/FooBar.cmake

add_fbthrift_cpp_library(
  fsdb_common_cpp2
  fboss/fsdb/if/fsdb_common.thrift
  OPTIONS
    json
)

add_fbthrift_cpp_library(
  fsdb_oper_cpp2
  fboss/fsdb/if/fsdb_oper.thrift
  OPTIONS
    json
  DEPENDS
    agent_config_cpp2
    agent_stats_cpp2
    fsdb_common_cpp2
    qsfp_config_cpp2
    qsfp_state_cpp2
    qsfp_stats_cpp2
    sensor_service_stats_cpp2
)
