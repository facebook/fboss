# CMake to build libraries and binaries for agent link testing

# In general, libraries and binaries in fboss/foo/bar are built by
# cmake/FooBar.cmake

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
  load_balancer_utils
  hw_qos_utils
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
  bcm_ecmp_utils
  bcm_qos_utils
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
    sai_ecmp_utils
    sai_qos_utils
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
