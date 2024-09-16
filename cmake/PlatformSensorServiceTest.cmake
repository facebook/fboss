# Make to build libraries and binaries in fboss/platform/weutils

# In general, libraries and binaries in fboss/foo/bar are built by
# cmake/FooBar.cmake

add_executable(sensor_service_sw_test
  fboss/platform/sensor_service/test/SensorServiceImplTest.cpp
  fboss/platform/sensor_service/test/SensorServiceThriftHandlerTest.cpp
  fboss/platform/sensor_service/test/TestUtils.cpp
)

target_link_libraries(sensor_service_sw_test
  platform_utils
  sensor_service_lib
  ${GTEST}
  ${LIBGMOCK_LIBRARIES}
)

install(TARGETS sensor_service_sw_test)

add_executable(sensor_service_utils_tests
  fboss/platform/sensor_service/test/UtilsTest.cpp
)

target_link_libraries(sensor_service_utils_tests
  sensor_service_utils
  ${GTEST}
  ${LIBGMOCK_LIBRARIES}
)
