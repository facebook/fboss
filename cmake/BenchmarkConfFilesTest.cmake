# CMake configuration for benchmark configuration files validation test

# Use system Python3 (/usr/bin/python3) which has pytest installed via pip
# in the Docker image. CMake's find_package(Python3) finds Python 3.12 built
# by getdeps.py, but pytest is only installed for the system Python 3.9.
set(BENCHMARK_TEST_PYTHON "/usr/bin/python3")

# Define the test
add_test(
  NAME benchmark_conf_files_test
  COMMAND ${BENCHMARK_TEST_PYTHON} -m pytest
    ${CMAKE_SOURCE_DIR}/fboss/oss/hw_benchmark_tests/test_benchmark_conf_files.py
    -v
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/fboss/oss/hw_benchmark_tests
)

# Set test properties
set_tests_properties(benchmark_conf_files_test PROPERTIES
  LABELS "unit;config_validation"
  TIMEOUT 30
)
