# CMake to build libraries and binaries in fboss/agent/test

# In general, libraries and binaries in fboss/foo/bar are built by
# cmake/FooBar.cmake

add_library(label_forwarding_utils
  fboss/agent/test/LabelForwardingUtils.cpp
)

target_link_libraries(label_forwarding_utils
  state
)

add_library(route_distribution_gen
  fboss/agent/test/RouteDistributionGenerator.cpp
)

add_library(resourcelibutil
  fboss/agent/test/ResourceLibUtil.cpp
)

target_link_libraries(resourcelibutil
  state
)

target_link_libraries(route_distribution_gen
  ecmp_helper
  resourcelibutil
  state
)

add_library(route_scale_gen
  fboss/agent/test/RouteScaleGenerators.cpp
)

target_link_libraries(route_scale_gen
  ecmp_helper
  route_distribution_gen
  state
)

add_library(ecmp_helper
  fboss/agent/test/EcmpSetupHelper.cpp
)

target_link_libraries(ecmp_helper
  switch_config_cpp2
  state
)

add_library(trunk_utils
  fboss/agent/test/TrunkUtils.cpp
)

target_link_libraries(trunk_utils
  switch_config_cpp2
  state
)

add_executable(async_logger_test
  fboss/agent/test/oss/Main.cpp
  fboss/agent/test/AsyncLoggerTest.cpp
)

target_link_libraries(async_logger_test
  async_logger
  ${GTEST}
  ${LIBGMOCK_LIBRARIES}
)

gtest_discover_tests(async_logger_test)
