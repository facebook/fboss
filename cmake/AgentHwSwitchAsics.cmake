# CMake to build libraries and binaries in fboss/agent/hw/switch_asics

# In general, libraries and binaries in fboss/foo/bar are built by
# cmake/FooBar.cmake

add_library(switch_asics
  fboss/agent/hw/switch_asics/FakeAsic.h
  fboss/agent/hw/switch_asics/HwAsic.cpp
  fboss/agent/hw/switch_asics/HwAsic.h
  fboss/agent/hw/switch_asics/Tomahawk4Asic.cpp
  fboss/agent/hw/switch_asics/Tomahawk3Asic.cpp
  fboss/agent/hw/switch_asics/TomahawkAsic.cpp
  fboss/agent/hw/switch_asics/Trident2Asic.cpp
  fboss/agent/hw/switch_asics/TajoAsic.cpp
)

target_link_libraries(switch_asics
  switch_config_cpp2
)
