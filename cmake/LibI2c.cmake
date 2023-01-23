# CMake to build libraries and binaries in fboss/lib/i2c

# In general, libraries and binaries in fboss/foo/bar are built by
# cmake/FooBar.cmake

add_library(i2c_ctrl
  fboss/lib/i2c/I2cDevIo.cpp
  fboss/lib/i2c/I2cRdWrIo.cpp
  fboss/lib/i2c/I2cSmbusIo.cpp
)

target_link_libraries(i2c_ctrl
  i2c_controller_stats_cpp2
  Folly::folly
)
