# CMake to build libraries and binaries in fboss/agent/hw/switch_asics

# In general, libraries and binaries in fboss/foo/bar are built by
# cmake/FooBar.cmake

add_library(version_util
    fboss/agent/hw/sai/impl/util.cpp
)

add_library(switch_asics
  fboss/agent/hw/switch_asics/FakeAsic.h
  fboss/agent/hw/switch_asics/EbroAsic.cpp
  fboss/agent/hw/switch_asics/YubaAsic.cpp
  fboss/agent/hw/switch_asics/HwAsic.cpp
  fboss/agent/hw/switch_asics/HwAsic.h
  fboss/agent/hw/switch_asics/Tomahawk5Asic.cpp
  fboss/agent/hw/switch_asics/Tomahawk4Asic.cpp
  fboss/agent/hw/switch_asics/Tomahawk3Asic.cpp
  fboss/agent/hw/switch_asics/TomahawkAsic.cpp
  fboss/agent/hw/switch_asics/Trident2Asic.cpp
  fboss/agent/hw/switch_asics/CredoPhyAsic.cpp
  fboss/agent/hw/switch_asics/Jericho2Asic.cpp
  fboss/agent/hw/switch_asics/Jericho3Asic.cpp
  fboss/agent/hw/switch_asics/RamonAsic.cpp
  fboss/agent/hw/switch_asics/Ramon3Asic.cpp
  fboss/agent/hw/switch_asics/BroadcomXgsAsic.cpp
)

target_link_libraries(switch_asics
  fboss_cpp2
  phy_cpp2
  switch_config_cpp2
  version_util
)
