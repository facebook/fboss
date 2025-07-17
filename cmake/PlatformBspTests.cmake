
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

add_library(bsp_test_environment
  fboss/platform/bsp_tests/cpp/BspTestEnvironment.cpp
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
)

target_link_libraries(bsp_tests
  ${GTEST}
  ${LIBGMOCK_LIBRARIES}
  bsp_test_environment
  platform_name_lib
  Folly::folly
)
