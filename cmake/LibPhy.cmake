# CMake to build libraries and binaries in fboss/lib/phy

# In general, libraries and binaries in fboss/foo/bar are built by
# cmake/FooBar.cmake

add_library(external_phy
  fboss/lib/phy/ExternalPhy.cpp
  fboss/lib/phy/ExternalPhyPortStatsUtils.cpp
)

target_link_libraries(external_phy
  ctrl_cpp2
  platform_config_cpp2
  phy_cpp2
)

add_library(phy_management_base
  fboss/lib/phy/PhyManager.cpp
)

target_link_libraries(phy_management_base
  external_phy
  error
  platform_mapping
  fboss_config_utils
  Folly::folly
)
