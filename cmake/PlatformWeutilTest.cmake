# Make to build libraries and binaries in fboss/platform/weutils

# In general, libraries and binaries in fboss/foo/bar are built by
# cmake/FooBar.cmake

add_executable(weutil_crc16_ccitt_test
  fboss/platform/weutil/test/Crc16ccittTest.cpp
)

target_link_libraries(weutil_crc16_ccitt_test
  weutil_crc16_ccitt_aug
  ${GTEST}
  ${LIBGMOCK_LIBRARIES}
)

add_executable(weutil_fboss_eeprom_parser_test
  fboss/platform/weutil/test/FbossEepromParserTest.cpp
)

target_link_libraries(weutil_fboss_eeprom_parser_test
  weutil_fboss_eeprom_parser
  weutil_lib
  Folly::folly
  ${GTEST}
  ${LIBGMOCK_LIBRARIES}
)
