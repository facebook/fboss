# CMake to build libraries and binaries in fboss/lib

# In general, libraries and binaries in fboss/foo/bar are built by
# cmake/FooBar.cmake

add_library(io_stats_recorder
  fboss/lib/IOStatsRecorder.cpp
)

target_link_libraries(io_stats_recorder
  io_stats_cpp2
)
