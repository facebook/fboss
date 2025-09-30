# Make to build libraries and binaries in fboss/platform/platform_hw_test

# In general, libraries and binaries in fboss/foo/bar are built by
# cmake/FooBar.cmake

add_executable(platform_hw_test
  fboss/platform/platform_hw_test/PlatformHwTest.cpp
)

target_link_libraries(platform_hw_test
  ${GTEST}
  ${LIBGMOCK_LIBRARIES}
  Folly::folly
  weutil_fboss_eeprom_interface
  weutil_lib
)
