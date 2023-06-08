# CMake to build libraries and binaries in fboss/lib/fpga

# In general, libraries and binaries in fboss/foo/bar are built by
# cmake/FooBar.cmake

add_library(fpga_multi_pim_container
  fboss/lib/fpga/MultiPimPlatformPimContainer.cpp
)

target_link_libraries(fpga_multi_pim_container
  fboss_error
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
)
