# Make to build libraries and binaries in fboss/platform/weutils

# In general, libraries and binaries in fboss/foo/bar are built by
# cmake/FooBar.cmake

add_executable(sensors_test
  fboss/platform/sensor_service/hw_test/Main.cpp
  fboss/platform/sensor_service/hw_test/SensorsTest.cpp
)

target_link_libraries(sensors_test
  sensor_service_lib
  ${GTEST}
  ${LIBGMOCK_LIBRARIES}
)
