# CMake to build libraries for MacsecUtils.cpp

# In general, libraries and binaries in fboss/foo/bar are built by
# cmake/FooBar.cmake

add_library(macsec_utils
  fboss/qsfp_service/util/MacsecUtils.cpp
)

target_link_libraries(macsec_utils
  mka_cpp2
  switch_config_cpp2
)
