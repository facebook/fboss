# CMake to build libraries and binaries in fboss/qsfp_service/test/hw_test

# In general, libraries and binaries in fboss/foo/bar are built by
# cmake/FooBar.cmake

add_library(hw_qsfp_ensemble
  fboss/qsfp_service/test/hw_test/HwQsfpEnsemble.cpp
)

target_link_libraries(hw_qsfp_ensemble
  platform_mapping
  resourcelibutil
  phy_management_base
  transceiver_manager
  qsfp_handler
  qsfp_core
)

add_library(hw_transceiver_utils
  fboss/qsfp_service/test/hw_test/HwTransceiverUtils.cpp
)

target_link_libraries(hw_transceiver_utils
  Folly::folly
  error
  switch_config_cpp2
  transceiver_cpp2
)

add_executable(qsfp_hw_test
  fboss/qsfp_service/test/hw_test/HwFirmwareTest.cpp
  fboss/qsfp_service/test/hw_test/HwPimTest.cpp
  fboss/qsfp_service/test/hw_test/HwPortProfileTest.cpp
  fboss/qsfp_service/test/hw_test/HwPortUtils.cpp
  fboss/qsfp_service/test/hw_test/HwTransceiverConfigTest.cpp
  fboss/qsfp_service/test/hw_test/HwStatsCollectionTest.cpp
  fboss/qsfp_service/test/hw_test/HwTest.cpp
  fboss/qsfp_service/test/hw_test/HwTransceiverResetTest.cpp
  fboss/qsfp_service/test/hw_test/HwExternalPhyPortTest.cpp
  fboss/qsfp_service/test/hw_test/HwPortPrbsTest.cpp
  fboss/qsfp_service/test/hw_test/HwI2CStressTest.cpp
  fboss/qsfp_service/test/hw_test/HwI2cSelectTest.cpp
  fboss/qsfp_service/test/hw_test/HwStateMachineTest.cpp
  fboss/qsfp_service/test/hw_test/HwTransceiverTest.cpp
  fboss/agent/test/oss/Main.cpp
)

target_link_libraries(qsfp_hw_test
  hw_qsfp_ensemble
  hw_transceiver_utils
  ${GTEST}
  ${LIBGMOCK_LIBRARIES}
  FBThrift::thriftcpp2
  fb303::fb303
  hw_test_main
)
