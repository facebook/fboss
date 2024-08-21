# Make to build libraries and binaries in fboss/platform/platform_manager

# In general, libraries and binaries in fboss/foo/bar are built by
# cmake/FooBar.cmake

add_executable(platform_manager_config_validator_test
  fboss/platform/platform_manager/tests/ConfigValidatorTest.cpp
)

target_link_libraries(platform_manager_config_validator_test
  platform_manager_config_validator
  Folly::folly
  ${GTEST}
  ${LIBGMOCK_LIBRARIES}
)
