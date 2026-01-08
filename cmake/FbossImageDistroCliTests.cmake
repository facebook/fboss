# CMake to build and test fboss-image/distro_cli

# In general, libraries and binaries in fboss/foo/bar are built by
# cmake/FooBar.cmake

include(FBPythonBinary)

file(GLOB DISTRO_CLI_TEST_SOURCES
  "fboss-image/distro_cli/tests/*_test.py"
)

file(GLOB DISTRO_CLI_TEST_HELPERS
  "fboss-image/distro_cli/tests/test_helpers.py"
)

file(GLOB_RECURSE DISTRO_CLI_LIB_SOURCES
  "fboss-image/distro_cli/builder/*.py"
  "fboss-image/distro_cli/cmds/*.py"
  "fboss-image/distro_cli/lib/*.py"
  "fboss-image/distro_cli/tools/*.py"
)

file(COPY "fboss-image/distro_cli/tests/data" DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/fboss-image/distro_cli/tests")

add_fb_python_unittest(
  distro_cli_tests
  BASE_DIR "fboss-image"
  SOURCES
    ${DISTRO_CLI_TEST_SOURCES}
    ${DISTRO_CLI_TEST_HELPERS}
    ${DISTRO_CLI_LIB_SOURCES}
  ENV
    "PYTHONPATH=${CMAKE_CURRENT_SOURCE_DIR}/fboss-image/distro_cli"
)

install_fb_python_executable(distro_cli_tests)
