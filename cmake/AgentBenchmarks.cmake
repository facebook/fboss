# CMake to build libraries and binaries in fboss/agent/benchmarks

# In general, libraries and binaries in fboss/foo/bar are built by
# cmake/FooBar.cmake

add_library(agent_benchmarks
  fboss/agent/benchmarks/AgentBenchmarks.cpp
)

target_link_libraries(agent_benchmarks
  agent_ensemble
  Folly::folly
)


add_library(bcm_agent_benchmarks_main
  fboss/agent/benchmarks/AgentBenchmarksMain.cpp
  fboss/agent/benchmarks/bcm/AgentBenchmarksMain.cpp
)

target_link_libraries(bcm_agent_benchmarks_main
  agent_benchmarks
  Folly::folly
  platform
)

add_library(sai_agent_benchmarks_main
  fboss/agent/benchmarks/AgentBenchmarksMain.cpp
  fboss/agent/benchmarks/sai/AgentBenchmarksMain.cpp
)

target_link_libraries(sai_agent_benchmarks_main
  agent_benchmarks
  Folly::folly
  sai_platform
)

set_target_properties(sai_agent_benchmarks_main PROPERTIES COMPILE_FLAGS
  "-DSAI_VER_MAJOR=${SAI_VER_MAJOR} \
  -DSAI_VER_MINOR=${SAI_VER_MINOR}  \
  -DSAI_VER_RELEASE=${SAI_VER_RELEASE}"
)
