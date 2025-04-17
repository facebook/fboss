# CMake to build libraries and binaries in fboss/lib/platform_mapping_v2

# In general, libraries and binaries in fboss/foo/bar are built by
# cmake/FooBar.cmake

include(FBPythonBinary)

set(
    PLATFORM_MAPPING_PY_SRCS
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

add_fb_python_executable(
    fboss-platform-mapping-gen
    MAIN_MODULE fboss.lib.platform_mapping_v2.gen:generate_platform_mappings
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

install_fb_python_executable(fboss-platform-mapping-gen)
