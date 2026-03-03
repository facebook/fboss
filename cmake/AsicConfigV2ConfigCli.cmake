# CMake to build libraries and binaries in fboss/lib/asic_config_v2

# In general, libraries and binaries in fboss/foo/bar are built by
# cmake/FooBar.cmake

include(FBPythonBinary)

set(
    ASIC_CONFIG_V2_PY_SRCS
    "fboss/lib/asic_config_v2/asic_config.py"
    "fboss/lib/asic_config_v2/bcm.py"
    "fboss/lib/asic_config_v2/bcmxgs.py"
    "fboss/lib/asic_config_v2/gen.py"
    "fboss/lib/asic_config_v2/all_asic_config_params.py"
    "fboss/lib/asic_config_v2/icecube800banw.py"
    "fboss/lib/asic_config_v2/icecube800bc.py"
    "fboss/lib/asic_config_v2/tomahawk6.py"
    "fboss/lib/asic_config_v2/montblanc.py"
    "fboss/lib/asic_config_v2/tomahawk5.py"
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
    fboss-asic-config-gen
    MAIN_MODULE fboss.lib.asic_config_v2.gen:generate_all_asic_configs
    SOURCES ${ASIC_CONFIG_V2_PY_SRCS}
    DEPENDS
        asic_config_v2_py
        platform_config_py
        switch_config_py
        transceiver_py
        phy_py
        platform_mapping_config_py
        fboss_common_py
        FBThrift::thrift_py
        python-pyyaml::python-pyyaml
)

install_fb_python_executable(fboss-asic-config-gen)
