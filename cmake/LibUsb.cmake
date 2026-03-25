# CMake to build libraries and binaries in fboss/lib/usb

# In general, libraries and binaries in fboss/foo/bar are built by
# cmake/FooBar.cmake

add_library(transceiver_access_parameter
  fboss/lib/usb/TransceiverAccessParameter.h
)

set_target_properties(transceiver_access_parameter PROPERTIES LINKER_LANGUAGE CXX)

add_library(i2_api
  fboss/lib/usb/TransceiverI2CApi.h
  fboss/lib/usb/TransceiverPlatformApi.h
  fboss/lib/usb/TransceiverPlatformI2cApi.h
)

target_link_libraries(i2_api
  transceiver_access_parameter
  i2c_controller_stats_cpp2
  Folly::folly
)

add_library(usb_api
  fboss/lib/usb/UsbError.h
)

target_link_libraries(usb_api
  i2_api
  Folly::folly
  ${USB}
)

# The name of usb might be too generic, use fboss_usb_lib instead
add_library(fboss_usb_lib
  fboss/lib/usb/UsbDevice.cpp
  fboss/lib/usb/UsbHandle.cpp
)

target_link_libraries(fboss_usb_lib
  usb_api
  Folly::folly
  ${USB}
)

add_library(cp2112
  fboss/lib/usb/CP2112.cpp
)

target_link_libraries(cp2112
  fboss_usb_lib
  usb_api
  bmc_rest_client
  i2c_ctrl
  Folly::folly
  ${USB}
)

add_library(base_i2c_dependencies
  fboss/lib/usb/BaseWedgeI2CBus.cpp
)

target_link_libraries(base_i2c_dependencies
  cp2112
  i2_api
  usb_api
  Folly::folly
)

# TODO Don't add minipack support cause xphy sdk is not open source ready
add_library(wedge_i2c
  fboss/lib/usb/GalaxyI2CBus.cpp
  fboss/lib/usb/PCA9548.cpp
  fboss/lib/usb/PCA9548MultiplexedBus.cpp
  fboss/lib/usb/PCA9548MuxedBus.cpp
  fboss/lib/usb/Wedge100I2CBus.cpp
  fboss/lib/usb/WedgeI2CBus.cpp
)

target_link_libraries(wedge_i2c
  base_i2c_dependencies
  cp2112
  i2_api
  usb_api
  fb_fpga_i2c
  i2c_controller_stats_cpp2
  platform_i2c_api
  Folly::folly
)
