# CMake to build libraries and binaries in fboss/lib/asic_config_v3
# Data-driven ASIC config generation

include(FBPythonBinary)

set(
    ASIC_CONFIG_V3_PY_SRCS
    "fboss/lib/asic_config_v3/__init__.py"
    "fboss/lib/asic_config_v3/base_generator.py"
    "fboss/lib/asic_config_v3/gen.py"
    "fboss/lib/asic_config_v3/generators/__init__.py"
)

add_fb_thrift_python_executable(
    fboss-asic-config-v3-gen
    MAIN_MODULE fboss.lib.asic_config_v3.gen:generate_all_asic_configs
    SOURCES ${ASIC_CONFIG_V3_PY_SRCS}
)

install_fb_python_executable(fboss-asic-config-v3-gen)
