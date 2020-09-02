# CMake to build libraries and binaries in fboss/agent/hw/bcm

# In general, libraries and binaries in fboss/foo/bar are built by
# cmake/FooBar.cmake

add_library(normalizer
  fboss/agent/normalization/Normalizer.cpp
)

target_link_libraries(normalizer
  hardware_stats_cpp2
  Folly::folly
)
