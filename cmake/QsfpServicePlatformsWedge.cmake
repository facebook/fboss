# CMake to build libraries and binaries in fboss/qsfp_service/platforms/wedge

# In general, libraries and binaries in fboss/foo/bar are built by
# cmake/FooBar.cmake

add_library(qsfp_platforms_wedge
  fboss/qsfp_service/platforms/wedge/BspWedgeManager.cpp
  fboss/qsfp_service/platforms/wedge/WedgeManager.cpp
  fboss/qsfp_service/platforms/wedge/WedgeQsfp.cpp
  fboss/qsfp_service/platforms/wedge/Wedge100Manager.cpp
  fboss/qsfp_service/platforms/wedge/GalaxyManager.cpp
  fboss/qsfp_service/platforms/wedge/Wedge40Manager.cpp
  fboss/qsfp_service/platforms/wedge/WedgeManagerInit.cpp
  fboss/qsfp_service/platforms/wedge/oss/WedgeManagerInit.cpp
)

target_link_libraries(qsfp_platforms_wedge
  qsfp_module
  fpga_multi_pim_container
  ledIO
  qsfp_bsp_core
  galaxy_platform_mapping
  wedge100_platform_mapping
  wedge40_platform_mapping
  kamet_platform_mapping
  makalu_platform_mapping
  platform_base
  qsfp_config
)
