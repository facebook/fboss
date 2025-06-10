# Make to build libraries and binaries in fboss/platform/config_lib

# In general, libraries and binaries in fboss/foo/bar are built by
# cmake/FooBar.cmake

add_executable(cross_config_validator_test
  fboss/platform/config_lib/tests/CrossConfigValidatorTest.cpp
)

target_link_libraries(cross_config_validator_test
  cross_config_validator
  ${GTEST}
  ${LIBGMOCK_LIBRARIES}
)
