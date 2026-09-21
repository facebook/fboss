# CMake to build libraries and binaries in fboss/lib/asic_config_v3/test
# Validation of generated ASIC config contents against checked-in references

include(FBPythonBinary)

# ASIC_CONFIG_V3_PY_SRCS comes from AsicConfigV3ConfigCli.cmake, which the
# top-level CMakeLists.txt globs in ahead of this file.
if(NOT DEFINED ASIC_CONFIG_V3_PY_SRCS)
    message(
        FATAL_ERROR
        "ASIC_CONFIG_V3_PY_SRCS is undefined; AsicConfigV3ConfigCli.cmake must "
        "be included before AsicConfigV3ValidateContentsCli.cmake"
    )
endif()

set(
    ASIC_CONFIG_V3_VALIDATE_CONTENTS_PY_SRCS
    "fboss/lib/asic_config_v3/test/validate_asic_configs_contents.py"
    ${ASIC_CONFIG_V3_PY_SRCS}
)

add_fb_thrift_python_executable(
    fboss-asic-config-v3-validate-contents
    MAIN_MODULE fboss.lib.asic_config_v3.test.validate_asic_configs_contents:main
    SOURCES ${ASIC_CONFIG_V3_VALIDATE_CONTENTS_PY_SRCS}
    DEPENDS
        platform_config_python
        switch_config_python
        transceiver_python
        phy_python
        platform_mapping_config_python
        fboss_common_python
        python-pyyaml::python-pyyaml
)

install_fb_python_executable(fboss-asic-config-v3-validate-contents)
