# CMake to build libraries and binaries from fboss/agent/PhySnapshotManager

# In general, libraries and binaries in fboss/foo/bar are built by
# cmake/FooBar.cmake

add_library(phy_snapshot_manager
  fboss/agent/PhySnapshotManager.cpp
)

target_link_libraries(phy_snapshot_manager
  phy_cpp2
  snapshot_manager
  state
)
