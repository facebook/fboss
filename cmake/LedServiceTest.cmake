# CMake to build libraries and binaries in fboss/led_service/hw_test

# In general, libraries and binaries in fboss/foo/bar are built by
# cmake/FooBar.cmake

add_executable(led_service_test
  fboss/led_service/hw_test/LedServiceTest.cpp
)

target_link_libraries(led_service_test
  led_manager_lib
  ${GTEST}
  ${LIBGMOCK_LIBRARIES}
)
