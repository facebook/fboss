# CMake to build libraries and binaries in fboss/lib/fpga

# In general, libraries and binaries in fboss/foo/bar are built by
# cmake/FooBar.cmake

add_library(fpga_multi_pim_container
  fboss/lib/fpga/MultiPimPlatformPimContainer.cpp
)

target_link_libraries(fpga_multi_pim_container
  fboss_error
)
