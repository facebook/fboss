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

add_executable(platform_manager_i2c_explorer_test
  fboss/platform/platform_manager/tests/I2cExplorerTest.cpp
)

target_link_libraries(platform_manager_i2c_explorer_test
  platform_manager_i2c_explorer
  platform_fs_utils
  ${GTEST}
  ${LIBGMOCK_LIBRARIES}
)

add_executable(platform_manager_platform_explorer_test
  fboss/platform/platform_manager/tests/PlatformExplorerTest.cpp
)

target_link_libraries(platform_manager_platform_explorer_test
  fb303::fb303
  platform_manager_platform_explorer
  platform_manager_utils
  Folly::folly
  ${GTEST}
  ${LIBGMOCK_LIBRARIES}
)

add_executable(platform_manager_utils_test
  fboss/platform/platform_manager/tests/UtilsTest.cpp
)

target_link_libraries(platform_manager_utils_test
  platform_manager_utils
  Folly::folly
  ${GTEST}
  ${LIBGMOCK_LIBRARIES}
)

add_executable(platform_manager_data_store_test
  fboss/platform/platform_manager/tests/DataStoreTest.cpp
)

target_link_libraries(platform_manager_data_store_test
  platform_manager_data_store
  ${GTEST}
  ${LIBGMOCK_LIBRARIES}
)

add_executable(platform_manager_device_path_resolver_test
  fboss/platform/platform_manager/tests/DevicePathResolverTest.cpp
)

target_link_libraries(platform_manager_device_path_resolver_test
  platform_manager_device_path_resolver
  ${GTEST}
  ${LIBGMOCK_LIBRARIES}
)

add_executable(platform_manager_presence_checker_test
  fboss/platform/platform_manager/tests/PresenceCheckerTest.cpp
)

target_link_libraries(platform_manager_presence_checker_test
  platform_manager_presence_checker
  platform_manager_utils
  platform_fs_utils
  ${GTEST}
  ${LIBGMOCK_LIBRARIES}
)
