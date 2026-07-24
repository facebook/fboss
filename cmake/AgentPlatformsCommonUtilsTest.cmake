# CMake to build tests in fboss/agent/platforms/common/utils/test

# In general, libraries and binaries in fboss/foo/bar are built by
# cmake/FooBar.cmake

add_executable(bcm_yaml_config_test
  fboss/agent/platforms/common/utils/test/BcmYamlConfigTest.cpp
)

target_link_libraries(bcm_yaml_config_test
  bcm_yaml_config
  ${GTEST}
  ${LIBGMOCK_LIBRARIES}
)

gtest_discover_tests(bcm_yaml_config_test)
