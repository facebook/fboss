# Make to build libraries and binaries in fboss/lib/platform_mapping_v2/test

# In general, libraries and binaries in fboss/foo/bar are built by
# cmake/FooBar.cmake

include(FBPythonBinary)

set(
    PLATFORM_MAPPING_PY_SRCS
    "fboss/lib/platform_mapping_v2/test/verify_generated_files.py"
    "fboss/lib/platform_mapping_v2/asic_vendor_config.py"
    "fboss/lib/platform_mapping_v2/gen.py"
    "fboss/lib/platform_mapping_v2/helpers.py"
    "fboss/lib/platform_mapping_v2/platform_mapping_v2.py"
    "fboss/lib/platform_mapping_v2/port_profile_mapping.py"
    "fboss/lib/platform_mapping_v2/profile_settings.py"
    "fboss/lib/platform_mapping_v2/read_files_utils.py"
    "fboss/lib/platform_mapping_v2/si_settings.py"
    "fboss/lib/platform_mapping_v2/static_mapping.py"
)

file(COPY "fboss/lib/platform_mapping_v2/platforms" DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/fboss/lib/platform_mapping_v2")
message(STATUS "Copying source files to: ${CMAKE_CURRENT_BINARY_DIR}/fboss/lib/platform_mapping_v2")

file(COPY "fboss/lib/platform_mapping_v2/generated_platform_mappings" DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/fboss/lib/platform_mapping_v2")
message(STATUS "Copying generated files to: ${CMAKE_CURRENT_BINARY_DIR}/fboss/lib/platform_mapping_v2")

add_fb_python_executable(
    platform_mapping_gen_no_regression_test
    MAIN_MODULE fboss.lib.platform_mapping_v2.test.verify_generated_files:run_tests
    SOURCES ${PLATFORM_MAPPING_PY_SRCS}
    DEPENDS
        asic_config_v2_py
        platform_config_py
        switch_config_py
        transceiver_py
        phy_py
        platform_mapping_config_py
        FBThrift::thrift_py
)

install_fb_python_executable(platform_mapping_gen_no_regression_test)
