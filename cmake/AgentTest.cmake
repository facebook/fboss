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
  core
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

add_library(agent_test_lib
  fboss/agent/test/AgentTest.cpp
)

target_link_libraries(agent_test_lib
  main
  fboss_agent
)

add_library(multinode_tests
  fboss/agent/test/MultiNodeTest.cpp
  fboss/agent/test/MultiNodeLacpTests.cpp
)

target_link_libraries(multinode_tests
  agent_test_lib
  main
  fboss_agent
  config_factory
  trunk_utils
  fboss_config_utils
  ${GTEST}
  ${LIBGMOCK_LIBRARIES}
)

add_library(link_tests
  fboss/agent/test/link_tests/LinkTest.cpp
  fboss/agent/test/link_tests/LinkSanityTests.cpp
)

target_link_libraries(link_tests
  agent_test_lib
  main
  fboss_agent
  config_factory
  fboss_config_utils
  ${GTEST}
  ${LIBGMOCK_LIBRARIES}
)

add_executable(bcm_link_test
  fboss/agent/test/link_tests/BcmLinkTests.cpp
)

target_link_libraries(bcm_link_test
  link_tests
  platform
  bcm
)

function(BUILD_SAI_LINK_TEST SAI_IMPL_NAME SAI_IMPL_ARG)

  message(STATUS "Building SAI_IMPL_NAME: ${SAI_IMPL_NAME} SAI_IMPL_ARG: ${SAI_IMPL_ARG}")

  add_executable(sai_link_test-${SAI_IMPL_NAME}-${SAI_VER_SUFFIX}
    fboss/agent/test/link_tests/SaiLinkTest.cpp
  )

  target_link_libraries(sai_link_test-${SAI_IMPL_NAME}-${SAI_VER_SUFFIX}
    # --whole-archive is needed for gtest to find these tests
    -Wl,--whole-archive
    ${SAI_IMPL_ARG}
    sai_switch
    link_tests
    agent_config_cpp2
    sai_platform
    -Wl,--no-whole-archive
    ref_map
    ${GTEST}
    ${LIBGMOCK_LIBRARIES}
  )

  set_target_properties(sai_link_test-${SAI_IMPL_NAME}-${SAI_VER_SUFFIX}
      PROPERTIES COMPILE_FLAGS
      "-DSAI_VER_MAJOR=${SAI_VER_MAJOR} \
      -DSAI_VER_MINOR=${SAI_VER_MINOR}  \
      -DSAI_VER_RELEASE=${SAI_VER_RELEASE}"
    )

endfunction()

BUILD_SAI_LINK_TEST("fake" fake_sai)
