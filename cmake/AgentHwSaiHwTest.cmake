# CMake to build libraries and binaries in fboss/agent/hw/sai/hw_test

# In general, libraries and binaries in fboss/foo/bar are built by
# cmake/FooBar.cmake

add_library(sai_switch_ensemble
  fboss/agent/hw/sai/hw_test/HwSwitchEnsembleFactory.cpp
  fboss/agent/hw/sai/hw_test/SaiSwitchEnsemble.cpp
  fboss/agent/hw/sai/hw_test/SaiLinkStateToggler.cpp
)

target_link_libraries(sai_switch_ensemble
  core
  setup_thrift
  sai_switch
  thrift_handler
  hw_switch_ensemble
  sai_platform
  sai_ctrl_cpp2
)

set_target_properties(sai_switch_ensemble PROPERTIES COMPILE_FLAGS
  "-DSAI_VER_MAJOR=${SAI_VER_MAJOR} \
  -DSAI_VER_MINOR=${SAI_VER_MINOR}  \
  -DSAI_VER_RELEASE=${SAI_VER_RELEASE}"
)

add_executable(sai_test-fake-${SAI_VER_MAJOR}.${SAI_VER_MINOR}.${SAI_VER_RELEASE}
  fboss/agent/hw/sai/hw_test/HwTestMacUtils.cpp
  fboss/agent/hw/sai/hw_test/HwVlanUtils.cpp
  fboss/agent/hw/sai/hw_test/HwTestCoppUtils.cpp
)

target_link_libraries(sai_test-fake-${SAI_VER_MAJOR}.${SAI_VER_MINOR}.${SAI_VER_RELEASE}
  # --whole-archive is needed for gtest to find these tests
  -Wl,--whole-archive
  sai_switch_ensemble
  hw_switch_test
  hw_test_main
  -Wl,--no-whole-archive
  ref_map
  fake_sai
  ${GTEST}
  ${LIBGMOCK_LIBRARIES}
)

set_target_properties(sai_test-fake-${SAI_VER_MAJOR}.${SAI_VER_MINOR}.${SAI_VER_RELEASE}
  PROPERTIES COMPILE_FLAGS
  "-DSAI_VER_MAJOR=${SAI_VER_MAJOR} \
  -DSAI_VER_MINOR=${SAI_VER_MINOR}  \
  -DSAI_VER_RELEASE=${SAI_VER_RELEASE}"
)
