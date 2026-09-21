# CMake to build libraries and binaries in fboss/mdio

# In general, libraries and binaries in fboss/foo/bar are built by
# cmake/FooBar.cmake

add_library(mdio
  fboss/mdio/Mdio.h
  fboss/mdio/MdioError.h
  fboss/mdio/Phy.h
)

target_link_libraries(mdio
  Folly::folly
  io_stats_recorder
  io_stats_cpp2
)

add_library(device_mdio
  fboss/mdio/BspDeviceMdio.cpp
)

target_link_libraries(device_mdio
  Folly::folly
  mdio
)
