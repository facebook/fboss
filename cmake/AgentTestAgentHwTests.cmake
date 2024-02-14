# CMake to build libraries and binaries in fboss/agent/hw/test

# In general, libraries and binaries in fboss/foo/bar are built by
# cmake/FooBar.cmake

add_library(agent_hw_test_src
  fboss/agent/test/agent_hw_tests/AgentEmptyTests.cpp
  fboss/agent/test/agent_hw_tests/AgentAclInDiscardCounterTests.cpp
  fboss/agent/test/agent_hw_tests/AgentInNullRouteDiscardsTest.cpp
  fboss/agent/test/agent_hw_tests/AgentPacketSendTests.cpp
  fboss/agent/test/agent_hw_tests/AgentVoqSwitchTests.cpp
  fboss/agent/test/agent_hw_tests/AgentFabricSwitchTests.cpp
)

target_link_libraries(agent_hw_test_src
  acl_test_utils
  copp_test_utils
  pkt_test_utils
  core
  hw_switch_fb303_stats
  config_factory
  agent_hw_test
  ecmp_helper
  fabric_test_utils
)

add_executable(multi_switch_agent_hw_test
  fboss/agent/test/agent_hw_tests/MultiSwitchAgentHwTest.cpp
)

target_link_libraries(multi_switch_agent_hw_test
  -Wl,--whole-archive
  agent_hw_test_src
  agent_hw_test
  multi_switch_agent_ensemble
  Folly::folly
  hw_packet_utils
  -Wl,--no-whole-archive
  ${GTEST}
  ${LIBGMOCK_LIBRARIES}
)

function(BUILD_SAI_AGENT_HW_TEST SAI_IMPL_NAME SAI_IMPL_ARG)

  message(STATUS "Building SAI_IMPL_NAME: ${SAI_IMPL_NAME} SAI_IMPL_ARG: ${SAI_IMPL_ARG}")
  add_executable(sai_agent_hw_test-${SAI_IMPL_NAME}
    fboss/agent/test/agent_hw_tests/SaiAgentHwTest.cpp
  )

  target_link_libraries(sai_agent_hw_test-${SAI_IMPL_NAME}
    -Wl,--whole-archive
    agent_hw_test_src
    ${SAI_IMPL_ARG}
    sai_acl_utils
    sai_copp_utils
    hw_packet_utils
    -Wl,--no-whole-archive
  )

set_target_properties(sai_agent_hw_test-${SAI_IMPL_NAME}
  PROPERTIES COMPILE_FLAGS
  "-DSAI_VER_MAJOR=${SAI_VER_MAJOR} \
  -DSAI_VER_MINOR=${SAI_VER_MINOR} \
  -DSAI_VER_RELEASE=${SAI_VER_RELEASE}"
)

endfunction()

if(BUILD_SAI_FAKE)
BUILD_SAI_AGENT_HW_TEST("fake" fake_sai)
install(
  TARGETS
  sai_agent_hw_test-fake)
endif()

# If libsai_impl is provided, build sai tests linking with it
find_library(SAI_IMPL sai_impl)
message(STATUS "SAI_IMPL: ${SAI_IMPL}")

if(SAI_IMPL)
  BUILD_SAI_AGENT_HW_TEST("sai_impl" ${SAI_IMPL})
  install(
    TARGETS
    sai_agent_hw_test-sai_impl)
endif()
