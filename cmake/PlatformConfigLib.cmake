# Make to build libraries and binaries in fboss/platform/config_lib

# In general, libraries and binaries in fboss/foo/bar are built by
# cmake/FooBar.cmake

add_library(platform_config_lib
  fboss/platform/config_lib/oss/ConfigLib.cpp
)
