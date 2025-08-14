
add_fbthrift_cpp_library(
  bsp_tests_config_cpp2
  fboss/platform/bsp_tests/bsp_tests_config.thrift
  OPTIONS
    json
    reflection
  DEPENDS
    fbiob_device_config_cpp2
    platform_manager_config_cpp2

)

add_fbthrift_cpp_library(
  fbiob_device_config_cpp2
  fboss/platform/bsp_tests/fbiob_device_config.thrift
  OPTIONS
    json
    reflection
)

add_library(bsp_test_utils
  fboss/platform/bsp_tests/cpp/utils/CdevUtils.cpp
  fboss/platform/bsp_tests/cpp/utils/GpioUtils.cpp
  fboss/platform/bsp_tests/cpp/utils/KmodUtils.cpp
  fboss/platform/bsp_tests/cpp/utils/I2CUtils.cpp
  fboss/platform/bsp_tests/cpp/utils/WatchdogUtils.cpp
)

target_link_libraries(bsp_test_utils
  fmt::fmt
  ${GTEST}
  bsp_tests_config_cpp2
  fbiob_device_config_cpp2
  platform_utils
  platform_manager_pkg_manager
  platform_manager_config_cpp2
  Folly::folly
  ${RE2}
)

add_library(bsp_test_environment
  fboss/platform/bsp_tests/cpp/BspTestEnvironment.cpp
  fboss/platform/bsp_tests/cpp/RuntimeConfigBuilder.cpp
)

target_link_libraries(bsp_test_environment
  ${GTEST}
  bsp_tests_config_cpp2
  platform_config_lib
  platform_manager_config_utils
  platform_manager_pkg_manager
  platform_manager_config_cpp2
  Folly::folly
  FBThrift::thriftcpp2
)

add_executable(bsp_tests
  fboss/platform/bsp_tests/cpp/BspTest.cpp
  fboss/platform/bsp_tests/cpp/BspTestRunner.cpp
  fboss/platform/bsp_tests/cpp/CdevTests.cpp
  fboss/platform/bsp_tests/cpp/GpioTests.cpp
  fboss/platform/bsp_tests/cpp/KmodTests.cpp
  fboss/platform/bsp_tests/cpp/I2CTests.cpp
  fboss/platform/bsp_tests/cpp/LedTests.cpp
  fboss/platform/bsp_tests/cpp/WatchdogTests.cpp
  fboss/platform/bsp_tests/cpp/XcvrTests.cpp
)

target_link_libraries(bsp_tests
  ${GTEST}
  ${LIBGMOCK_LIBRARIES}
  bsp_test_environment
  bsp_test_utils
  platform_name_lib
  platform_manager_config_cpp2
  Folly::folly
  ${GFLAGS}
)

add_executable(runtime_config_builder_test
  fboss/platform/bsp_tests/cpp/RuntimeConfigBuilderTest.cpp
)

target_link_libraries(runtime_config_builder_test
  ${GTEST}
  ${LIBGMOCK_LIBRARIES}
  bsp_test_environment
  Folly::folly
  FBThrift::thriftcpp2
)
