# Make to build libraries and binaries in fboss/platform/fan_service

# In general, libraries and binaries in fboss/foo/bar are built by
# cmake/FooBar.cmake

add_library(fan_service_hw_tests
  fboss/platform/fan_service/hw_test/FanServiceTest.cpp
)

target_link_libraries(fan_service_hw_tests
  fan_service_lib
  Folly::folly
  ${GTEST}
  ${LIBGMOCK_LIBRARIES}
)
