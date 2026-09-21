# CMake to build libraries and binaries in fboss/qsfp_service/module/tests

# In general, libraries and binaries in fboss/foo/bar are built by
# cmake/FooBar.cmake

add_library(fake_transceiver_impl
  fboss/qsfp_service/module/tests/FakeTransceiverImpl.cpp
)

target_link_libraries(fake_transceiver_impl
  ${GTEST}
  i2_api
  transceiver_manager
  firmware_upgrader
  qsfp_module
)

add_library(transceiver_tests_helper
  fboss/qsfp_service/module/tests/TransceiverTestsHelper.cpp
)

target_link_libraries(transceiver_tests_helper
  ${GTEST}
  qsfp_module
)

add_executable(cmis_test
  fboss/util/oss/TestMain.cpp
  fboss/qsfp_service/module/tests/CmisTest.cpp
)

target_link_libraries(cmis_test
  ${GTEST}
  ${LIBGMOCK_LIBRARIES}
  fake_transceiver_impl
  transceiver_tests_helper
  qsfp_module
  transceiver_properties_manager
  transceiver_manager_test_helper
  hw_transceiver_utils
)

gtest_discover_tests(cmis_test)
