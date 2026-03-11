# CMake to build libraries and binaries in fboss/qsfp_service/test/hal_test

# In general, libraries and binaries in fboss/foo/bar are built by
# cmake/FooBar.cmake

add_library(bsp_transceiver_impl
  fboss/qsfp_service/test/hal_test/BspTransceiverImpl.cpp
)

target_link_libraries(bsp_transceiver_impl
  qsfp_bsp_core
  bsp_platform_mapping_cpp2
  firmware_upgrader
)

add_library(hal_test_utils
  fboss/qsfp_service/test/hal_test/HalTestUtils.cpp
)

target_link_libraries(hal_test_utils
  bsp_transceiver_impl
  hal_test_config_cpp2
  fboss_error
  bsp_platform_mapping_cpp2
  qsfp_module
  Folly::folly
  FBThrift::thriftcpp2
)

add_library(hal_test_base
  fboss/qsfp_service/test/hal_test/HalTest.cpp
)

target_link_libraries(hal_test_base
  ${GTEST}
  hal_test_config_cpp2
  hal_test_utils
  fboss_error
  qsfp_module
  Folly::folly
)

add_executable(qsfp_hal_test
  fboss/qsfp_service/test/hal_test/HalTestModuleAdvertisement.cpp
  fboss/qsfp_service/test/hal_test/HalTestModuleInit.cpp
  fboss/qsfp_service/test/hal_test/Main.cpp
)

target_link_libraries(qsfp_hal_test
  ${GTEST}
  hal_test_base
  transceiver_cpp2
  qsfp_module
  fboss_init
  Folly::folly
)

install(TARGETS qsfp_hal_test)
