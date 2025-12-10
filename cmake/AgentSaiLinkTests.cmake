# CMake to build libraries and binaries for agent link testing

function(BUILD_SAI_LINK_TEST SAI_IMPL_NAME SAI_IMPL_ARG)

  message(STATUS "Building SAI_IMPL_NAME: ${SAI_IMPL_NAME} SAI_IMPL_ARG: ${SAI_IMPL_ARG}")

  add_executable(sai_link_test-${SAI_IMPL_NAME}
    fboss/agent/hw/sai/hw_test/HwTestFabricUtils.cpp
    fboss/agent/test/link_tests/SaiLinkTest.cpp
  )

  add_sai_sdk_dependencies(sai_link_test-${SAI_IMPL_NAME})

  target_link_libraries(sai_link_test-${SAI_IMPL_NAME}
    # --whole-archive is needed for gtest to find these tests
    -Wl,--whole-archive
    ${SAI_IMPL_ARG}
    link_tests
    agent_config_cpp2
    sai_platform
    sai_ecmp_utils
    sai_port_utils
    sai_traced_api
    setup_thrift_prod
    trap_packet_utils
    -Wl,--no-whole-archive
    ref_map
    ${GTEST}
    ${LIBGMOCK_LIBRARIES}
  )

  set_target_properties(sai_link_test-${SAI_IMPL_NAME}
      PROPERTIES COMPILE_FLAGS
      "-DSAI_VER_MAJOR=${SAI_VER_MAJOR} \
      -DSAI_VER_MINOR=${SAI_VER_MINOR}  \
      -DSAI_VER_RELEASE=${SAI_VER_RELEASE}"
    )

endfunction()

if(BUILD_SAI_FAKE AND BUILD_SAI_FAKE_LINK_TEST)
  BUILD_SAI_LINK_TEST("fake" fake_sai)
  install(
  TARGETS
  sai_link_test-fake)
endif()

# If libsai_impl is provided, build link test linking with it
find_library(SAI_IMPL sai_impl)
message(STATUS "SAI_IMPL: ${SAI_IMPL}")

if(SAI_IMPL)
  BUILD_SAI_LINK_TEST("sai_impl" ${SAI_IMPL})
  install(
    TARGETS
    sai_link_test-sai_impl)
endif()
