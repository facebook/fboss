# Make to build libraries and binaries in fboss/platform/weutils

# In general, libraries and binaries in fboss/foo/bar are built by
# cmake/FooBar.cmake

add_fbthrift_cpp_library(
  weutil_config_cpp2
  fboss/platform/weutil/if/weutil_config.thrift
  OPTIONS
    json
    reflection
)

add_library(weutil_fboss_eeprom_parser
  fboss/platform/weutil/FbossEepromParser.cpp
  fboss/platform/weutil/Crc16CcittAug.cpp
)

target_link_libraries(weutil_fboss_eeprom_parser
  Folly::folly
)

add_library(weutil_lib
  fboss/platform/weutil/WeutilDarwin.cpp
  fboss/platform/weutil/WeutilImpl.cpp
  fboss/platform/weutil/prefdl/Prefdl.cpp
  fboss/platform/weutil/Weutil.cpp
)

target_link_libraries(weutil_lib
  product_info
  platform_utils
  Folly::folly
  weutil_config_cpp2
  weutil_fboss_eeprom_parser
  platform_config_lib
)

add_executable(weutil
  fboss/platform/weutil/main.cpp
)

target_link_libraries(weutil
  weutil_lib
)

install(TARGETS weutil)
