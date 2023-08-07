# CMake to build libraries and binaries in fboss/lib

# In general, libraries and binaries in fboss/foo/bar are built by
# cmake/FooBar.cmake

add_library(radix_tree
  fboss/lib/RadixTree.h
  fboss/lib/RadixTree-inl.h
)

target_link_libraries(radix_tree
  Folly::folly
)

add_library(log_thrift_call
  fboss/lib/LogThriftCall.cpp
  fboss/lib/oss/LogThriftCall.cpp
)

target_link_libraries(log_thrift_call
  Folly::folly
  FBThrift::thriftcpp2
)

add_library(alert_logger
  fboss/lib/AlertLogger.cpp
)

target_link_libraries(alert_logger
  Folly::folly
)

add_library(ref_map
  fboss/lib/RefMap.h
)

set_target_properties(ref_map PROPERTIES LINKER_LANGUAGE CXX)

add_library(tuple_utils
  fboss/lib/TupleUtils.h
)

set_target_properties(tuple_utils PROPERTIES LINKER_LANGUAGE CXX)

add_library(exponential_back_off
  fboss/lib/ExponentialBackoff.cpp
)

target_link_libraries(exponential_back_off
  Folly::folly
)

add_library(function_call_time_reporter
  fboss/lib/FunctionCallTimeReporter.cpp
)

target_link_libraries(function_call_time_reporter
  Folly::folly
)

add_library(fboss_i2c_lib
  fboss/lib/usb/GalaxyI2CBus.cpp
  fboss/lib/usb/BaseWedgeI2CBus.cpp
  fboss/lib/usb/BaseWedgeI2CBus.h
  fboss/lib/RestClient.cpp
  fboss/lib/BmcRestClient.cpp
  fboss/lib/usb/CP2112.cpp
  fboss/lib/usb/CP2112.h
  fboss/lib/usb/PCA9548.cpp
  fboss/lib/usb/PCA9548MultiplexedBus.cpp
  fboss/lib/usb/PCA9548MuxedBus.cpp
  fboss/lib/i2c/PCA9541.cpp
  fboss/lib/i2c/PCA9541.h
  fboss/lib/usb/TransceiverI2CApi.h
  fboss/lib/usb/UsbDevice.cpp
  fboss/lib/usb/UsbDevice.h
  fboss/lib/usb/UsbError.h
  fboss/lib/usb/UsbHandle.cpp
  fboss/lib/usb/UsbHandle.h
  fboss/lib/usb/Wedge100I2CBus.cpp
  fboss/lib/usb/Wedge100I2CBus.h
  fboss/lib/usb/WedgeI2CBus.cpp
  fboss/lib/usb/WedgeI2CBus.h
)

target_link_libraries(fboss_i2c_lib
  agent_config_cpp2
  switch_config_cpp2
  hardware_stats_cpp2
  i2c_controller_stats_cpp2
  qsfp_cpp2
  Folly::folly
  fb303::fb303
  FBThrift::thriftcpp2
  ${USB}
  ${CURL}
  wedge40_platform_mapping
  wedge100_platform_mapping
  galaxy_platform_mapping
)

add_library(common_file_utils
  fboss/lib/CommonFileUtils.cpp
)

target_link_libraries(common_file_utils
  fboss_error
)

add_library(common_port_utils
  fboss/lib/CommonPortUtils.cpp
)

target_link_libraries(common_port_utils
  fboss_error
)

add_library(common_utils
  fboss/lib/CommonUtils.h
)

target_link_libraries(common_utils
  fboss_error
  fb303::fb303
)

add_library(thread_heartbeat
  fboss/lib/ThreadHeartbeat.cpp
)

target_link_libraries(thread_heartbeat
  Folly::folly
)

add_library(pci_device
  fboss/lib/PciDevice.cpp
  fboss/lib/PciSystem.cpp
)

target_link_libraries(pci_device
  Folly::folly
)

add_library(physical_memory
  fboss/lib/PhysicalMemory.cpp
)

target_link_libraries(physical_memory
  Folly::folly
)

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

add_library(pci_access
  fboss/lib/PciAccess.cpp
)


add_library(hw_write_behavior
  fboss/lib/HwWriteBehavior.cpp
)

target_link_libraries(hw_write_behavior)
