# CMake to build libraries and binaries in fboss/agent/hw/bcm

# In general, libraries and binaries in fboss/foo/bar are built by
# cmake/FooBar.cmake

add_library(hw_benchmark_main
  fboss/lib/test/benchmarks/HwBenchmarkMain.cpp
)

target_link_libraries(hw_benchmark_main
  Folly::folly
  Folly::follybenchmark
)
