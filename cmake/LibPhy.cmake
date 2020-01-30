# CMake to build libraries and binaries in fboss/lib/phy

# In general, libraries and binaries in fboss/foo/bar are built by
# cmake/FooBar.cmake

add_library(external_phy
  fboss/lib/phy/ExternalPhy.cpp
)

target_link_libraries(external_phy
  platform_config_cpp2
  phy_cpp2
)
