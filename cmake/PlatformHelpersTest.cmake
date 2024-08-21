# Make to build libraries and binaries in fboss/platform/helpers

# In general, libraries and binaries in fboss/foo/bar are built by
# cmake/FooBar.cmake

add_executable(platform_helpers_platform_name_lib_test
  fboss/platform/helpers/tests/PlatformNameLibTest.cpp
)

target_link_libraries(platform_helpers_platform_name_lib_test
  platform_utils
  platform_name_lib
  ${GTEST}
  ${LIBGMOCK_LIBRARIES}
)
