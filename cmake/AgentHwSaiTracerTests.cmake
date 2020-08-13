# CMake to build libraries and binaries in fboss/agent/hw/sai/tracer/tests

# In general, libraries and binaries in fboss/foo/bar are built by
# cmake/FooBar.cmake

add_executable(async_logger_test
  fboss/agent/test/oss/Main.cpp
  fboss/agent/hw/sai/tracer/tests/AsyncLoggerTest.cpp
)

target_link_libraries(async_logger_test
  async_logger
  ${GTEST}
  ${LIBGMOCK_LIBRARIES}
)

gtest_discover_tests(async_logger_test)
