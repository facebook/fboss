# CMake to build libraries and binaries in fboss/platform/platform_checks

# In general, libraries and binaries in fboss/foo/bar are built by
# cmake/FooBar.cmake

add_fbthrift_cpp_library(
  check_types_cpp2
  fboss/platform/platform_checks/check_types.thrift
  OPTIONS
    json
    reflection
)

add_library(platform_check
  fboss/platform/platform_checks/PlatformCheck.cpp
)

target_link_libraries(platform_check
  check_types_cpp2
  platform_manager_config_cpp2
)

add_library(platform_checks
  fboss/platform/platform_checks/checks/MacAddressCheck.cpp
  fboss/platform/platform_checks/checks/PciDeviceCheck.cpp
  fboss/platform/platform_checks/checks/PowerResetCheck.cpp
  # Not including KernelVersionCheck since it relies on internal tools
)

target_link_libraries(platform_checks
  platform_check
  platform_fs_utils
  platform_manager_config_cpp2
  weutil_fboss_eeprom_interface
  weutil_config_utils
  Folly::folly
  ${RE2}
  platform_utils
)

add_executable(mac_address_check_test
  fboss/platform/platform_checks/tests/MacAddressCheckTest.cpp
)

target_link_libraries(mac_address_check_test
  platform_checks
  ${GTEST}
  ${LIBGMOCK_LIBRARIES}
)

gtest_discover_tests(mac_address_check_test)

add_executable(pci_device_check_test
  fboss/platform/platform_checks/tests/PciDeviceCheckTest.cpp
)

target_link_libraries(pci_device_check_test
  platform_checks
  ${GTEST}
  ${LIBGMOCK_LIBRARIES}
)

gtest_discover_tests(pci_device_check_test)
