# CMake to build libraries and binaries in fboss/agent/benchmarks

# In general, libraries and binaries in fboss/foo/bar are built by
# cmake/FooBar.cmake

add_library(mono_agent_benchmarks
  fboss/agent/benchmarks/AgentBenchmarks.cpp
)

target_link_libraries(mono_agent_benchmarks
  mono_agent_ensemble
  Folly::folly
)

add_library(mono_sai_agent_benchmarks_main
  fboss/agent/benchmarks/AgentBenchmarksMain.cpp
  fboss/agent/benchmarks/sai/AgentBenchmarksMain.cpp
)

target_link_libraries(mono_sai_agent_benchmarks_main
  mono_agent_benchmarks
  Folly::folly
  sai_platform
)

set_target_properties(mono_sai_agent_benchmarks_main PROPERTIES COMPILE_FLAGS
  "-DSAI_VER_MAJOR=${SAI_VER_MAJOR} \
  -DSAI_VER_MINOR=${SAI_VER_MINOR}  \
  -DSAI_VER_RELEASE=${SAI_VER_RELEASE}"
)

add_library(multi_switch_mono_sai_agent_benchmarks_main
  fboss/agent/benchmarks/AgentBenchmarksMain.cpp
  fboss/agent/benchmarks/sai/AgentBenchmarksMain.cpp
)

target_link_libraries(multi_switch_mono_sai_agent_benchmarks_main
  multi_switch_agent_benchmarks
  Folly::folly
  sai_platform
)

set_target_properties(multi_switch_mono_sai_agent_benchmarks_main PROPERTIES COMPILE_FLAGS
  "-DSAI_VER_MAJOR=${SAI_VER_MAJOR} \
  -DSAI_VER_MINOR=${SAI_VER_MINOR}  \
  -DSAI_VER_RELEASE=${SAI_VER_RELEASE}"
)
