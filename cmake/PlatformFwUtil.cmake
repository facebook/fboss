# Make to build libraries and binaries in fboss/platform/weutils

# In general, libraries and binaries in fboss/foo/bar are built by
# cmake/FooBar.cmake

add_executable(fw_util
  fboss/platform/fw_util/darwinFwUtil/FirmwareUpgradeDarwin.cpp
  fboss/platform/fw_util/fw_util.cpp
  fboss/platform/fw_util/darwinFwUtil/UpgradeBinaryDarwin.cpp
  fboss/platform/fw_util/firmware_helpers/FirmwareUpgradeHelper.cpp
  fboss/platform/fw_util/oss/FirmwareUpgradeHelper.cpp
)

target_link_libraries(fw_util
  Folly::folly
  platform_utils
  common_file_utils
)

install(TARGETS fw_util)
