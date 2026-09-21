# CMake to build libraries and binaries in fboss/platform/fixmyfboss

# In general, libraries and binaries in fboss/foo/bar are built by
# cmake/FooBar.cmake

add_library(result_printer
  fboss/platform/fixmyfboss/ResultPrinter.cpp
)

target_link_libraries(result_printer
  check_types_cpp2
)

add_executable(fixmyfboss
  fboss/platform/fixmyfboss/main.cpp
)

target_link_libraries(fixmyfboss
  result_printer
  platform_name_lib
  platform_checks
  CLI11::CLI11
)
