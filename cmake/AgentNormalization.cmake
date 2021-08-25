# CMake to build libraries and binaries in fboss/agent/hw/bcm

# In general, libraries and binaries in fboss/foo/bar are built by
# cmake/FooBar.cmake

add_library(normalizer
  fboss/agent/normalization/Normalizer.cpp
  fboss/agent/normalization/oss/Normalizer.cpp
  fboss/agent/normalization/PortStatsProcessor.cpp
  fboss/agent/normalization/TransformHandler.cpp
  fboss/agent/normalization/StatsExporter.cpp
  fboss/agent/normalization/CounterTagManager.cpp
)

target_link_libraries(normalizer
  hardware_stats_cpp2
  Folly::folly
  fb303::fb303
  fboss_types
  switch_config_cpp2
)
