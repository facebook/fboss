# CMake to build libraries and binaries in fboss/lib/fpga

# In general, libraries and binaries in fboss/foo/bar are built by
# cmake/FooBar.cmake

add_library(fpga_device
  fboss/lib/fpga/FpgaDevice.cpp
  fboss/lib/fpga/HwMemoryRegion.h
  fboss/lib/fpga/HwMemoryRegister.h
)

target_link_libraries(fpga_device
  pci_device
  physical_memory
  fboss_types
  Folly::folly
)

add_library(fpga_multi_pim_container
  fboss/lib/fpga/MultiPimPlatformPimContainer.cpp
)

target_link_libraries(fpga_multi_pim_container
  fboss_error
  pim_state_cpp2
)

add_library(facebook_fpga
  fboss/lib/fpga/FbDomFpga.cpp
  fboss/lib/fpga/FbFpgaRegisters.cpp
)

target_link_libraries(facebook_fpga
  fboss_error
  Folly::folly
  fpga_device
)

add_library(fb_fpga_i2c
  fboss/lib/fpga/FbFpgaI2c.cpp
)

target_link_libraries(fb_fpga_i2c
  facebook_fpga
  utils
  Folly::folly
  i2c_controller_stats_cpp2
)

add_library(fb_fpga_spi
  fboss/lib/fpga/FbFpgaSpi.h
  fboss/lib/fpga/FbFpgaSpi.cpp
)

target_link_libraries(fb_fpga_spi
  facebook_fpga
  utils
  i2c_ctrl
  i2_api
  Folly::folly
)

add_library(wedge400_fpga
  fboss/lib/fpga/Wedge400Fpga.cpp
)

target_link_libraries(wedge400_fpga
  fboss_error
  fboss_types
  Folly::folly
  pci_access
  facebook_fpga
)

add_library(wedge400_i2c
  fboss/lib/fpga/Wedge400I2CBus.cpp
  fboss/lib/fpga/Wedge400TransceiverApi.cpp
)

target_link_libraries(wedge400_i2c
  wedge400_fpga
  pci_access
  Folly::folly
  utils
  fb_fpga_i2c
)
