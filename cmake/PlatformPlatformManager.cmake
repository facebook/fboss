# Make to build libraries and binaries in fboss/platform/platform_manager

# In general, libraries and binaries in fboss/foo/bar are built by
# cmake/FooBar.cmake

add_fbthrift_cpp_library(
  platform_manager_snapshot_cpp2
  fboss/platform/platform_manager/platform_manager_snapshot.thrift
  OPTIONS
    json
    reflection
)

add_fbthrift_cpp_library(
  platform_manager_presence_cpp2
  fboss/platform/platform_manager/platform_manager_presence.thrift
  OPTIONS
    json
    reflection
)

add_fbthrift_cpp_library(
  platform_manager_config_cpp2
  fboss/platform/platform_manager/platform_manager_config.thrift
  OPTIONS
    json
    reflection
  DEPENDS
    platform_manager_presence_cpp2
)

add_fbthrift_cpp_library(
  platform_manager_service_cpp2
  fboss/platform/platform_manager/platform_manager_service.thrift
  SERVICES
    PlatformManagerService
  OPTIONS
    json
    reflection
  DEPENDS
   platform_manager_snapshot_cpp2
   platform_manager_config_cpp2
)

add_library(platform_manager_i2c_explorer
  fboss/platform/platform_manager/I2cExplorer.cpp
)

target_link_libraries(platform_manager_i2c_explorer
  fmt::fmt
  platform_manager_config_cpp2
  platform_manager_utils
  i2c_ctrl
  platform_utils
  Folly::folly
  ${RE2}
)

add_library(platform_manager_data_store
  fboss/platform/platform_manager/DataStore.cpp
)

target_link_libraries(platform_manager_data_store
  fmt::fmt
  platform_manager_config_cpp2
  Folly::folly
)

add_library(platform_manager_utils
  fboss/platform/platform_manager/Utils.cpp
)

target_link_libraries(platform_manager_utils
  platform_manager_config_validator
  platform_manager_config_cpp2
  gpiod_line
  platform_config_lib
  platform_name_lib
  Folly::folly
)

add_library(platform_manager_presence_checker
  fboss/platform/platform_manager/PresenceChecker.cpp
)

target_link_libraries(platform_manager_presence_checker
  platform_manager_device_path_resolver
  platform_manager_utils
)

add_library(platform_manager_pci_explorer
  fboss/platform/platform_manager/PciExplorer.cpp
)

target_link_libraries(platform_manager_pci_explorer
  platform_manager_i2c_explorer
  platform_manager_config_cpp2
  platform_manager_utils
  Folly::folly
)

add_library(platform_manager_device_path_resolver
  fboss/platform/platform_manager/DevicePathResolver.cpp
)

target_link_libraries(platform_manager_device_path_resolver
  platform_manager_data_store
  platform_manager_i2c_explorer
  platform_manager_pci_explorer
  platform_manager_config_cpp2
  platform_manager_utils
)

add_library(platform_manager_platform_explorer
  fboss/platform/platform_manager/PlatformExplorer.cpp
  fboss/platform/platform_manager/ExplorationErrorMap.cpp
)

target_link_libraries(platform_manager_platform_explorer
  platform_manager_data_store
  platform_manager_device_path_resolver
  platform_manager_i2c_explorer
  platform_manager_pci_explorer
  platform_manager_config_cpp2
  platform_manager_service_cpp2
  platform_manager_presence_checker
  platform_manager_utils
  platform_fs_utils
  fb303::fb303
  weutil_fboss_eeprom_parser
  ioctl_smbus_eeprom_reader
  Folly::folly
)

add_library(platform_manager_config_validator
  fboss/platform/platform_manager/ConfigValidator.cpp
)

target_link_libraries(platform_manager_config_validator
  platform_manager_i2c_explorer
  platform_manager_config_cpp2
  Folly::folly
)

add_executable(platform_manager
  fboss/platform/platform_manager/ConfigValidator.cpp
  fboss/platform/platform_manager/DataStore.cpp
  fboss/platform/platform_manager/I2cExplorer.cpp
  fboss/platform/platform_manager/Main.cpp
  fboss/platform/platform_manager/PciExplorer.cpp
  fboss/platform/platform_manager/PkgManager.cpp
  fboss/platform/platform_manager/PlatformExplorer.cpp
  fboss/platform/platform_manager/PlatformManagerHandler.cpp
  fboss/platform/platform_manager/DevicePathResolver.cpp
  fboss/platform/platform_manager/Utils.cpp
  fboss/platform/platform_manager/PresenceChecker.cpp
  fboss/platform/platform_manager/ExplorationErrorMap.cpp
)

target_link_libraries(platform_manager
  fb303::fb303
  platform_config_lib
  platform_fs_utils
  platform_name_lib
  platform_utils
  platform_manager_config_cpp2
  platform_manager_presence_cpp2
  platform_manager_service_cpp2
  platform_manager_snapshot_cpp2
  weutil_fboss_eeprom_parser
  ioctl_smbus_eeprom_reader
  i2c_ctrl
  ${LIBGPIOD}
  gpiod_line
)

install(TARGETS platform_manager)
