# CMake to build libraries and binaries in fboss/util

# In general, libraries and binaries in fboss/foo/bar are built by
# cmake/FooBar.cmake

add_executable(wedge_qsfp_util
  fboss/util/wedge_qsfp_util.cpp
  fboss/util/qsfp_util_main.cpp
  fboss/util/qsfp/QsfpServiceDetector.cpp
  fboss/util/qsfp/QsfpUtilContainer.cpp
  fboss/util/qsfp/QsfpUtilTx.cpp
  fboss/util/oss/wedge_qsfp_util.cpp
)

target_link_libraries(wedge_qsfp_util
  firmware_upgrader
  qsfp_config
  qsfp_lib
  qsfp_module
  ${YAML-CPP}
  phy_management_base
  transceiver_manager
  qsfp_platforms_wedge
)
