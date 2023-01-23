# CMake to build libraries and binaries in fboss/agent/benchmarks

# In general, libraries and binaries in fboss/foo/bar are built by
# cmake/FooBar.cmake

add_library(agent_benchmarks
  fboss/agent/benchmarks/AgentBenchmarks.cpp
)

target_link_libraries(agent_benchmarks
  agent_ensemble
)
