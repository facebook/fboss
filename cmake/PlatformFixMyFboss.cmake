# CMake to build libraries and binaries in fboss/platform/fixmyfboss

# In general, libraries and binaries in fboss/foo/bar are built by
# cmake/FooBar.cmake

add_library(result_printer
  fboss/platform/fixmyfboss/ResultPrinter.cpp
)

target_link_libraries(result_printer
  check_types_cpp2
)

add_library(fixmyfboss_lib
  fboss/platform/fixmyfboss/CheckRegistry.cpp
  fboss/platform/fixmyfboss/CheckRunner.cpp
)

target_link_libraries(fixmyfboss_lib
  platform_checks
  platform_checks_host
)

add_executable(fixmyfboss
  fboss/platform/fixmyfboss/main.cpp
)

target_link_libraries(fixmyfboss
  fixmyfboss_lib
  result_printer
  platform_checks_platform_name
  CLI11::CLI11
)

add_executable(fixmyfboss_result_printer_test
  fboss/platform/fixmyfboss/tests/ResultPrinterTest.cpp
)

target_link_libraries(fixmyfboss_result_printer_test
  result_printer
  ${GTEST}
  ${LIBGMOCK_LIBRARIES}
)

gtest_discover_tests(fixmyfboss_result_printer_test)

add_executable(fixmyfboss_check_runner_test
  fboss/platform/fixmyfboss/tests/CheckRunnerTest.cpp
)

target_link_libraries(fixmyfboss_check_runner_test
  fixmyfboss_lib
  ${GTEST}
  ${LIBGMOCK_LIBRARIES}
)

gtest_discover_tests(fixmyfboss_check_runner_test)
