# CMake to build libraries and binaries in fboss/agent/platforms/sai

# In general, libraries and binaries in fboss/foo/bar are built by
# cmake/FooBar.cmake

add_library(sai_platform
  fboss/agent/platforms/sai/SaiPlatform.cpp
  fboss/agent/platforms/sai/SaiBcmPlatform.cpp
  fboss/agent/platforms/sai/SaiBcmGalaxyFCPlatform.cpp
  fboss/agent/platforms/sai/SaiBcmGalaxyLCPlatform.cpp
  fboss/agent/platforms/sai/SaiBcmWedge100Platform.cpp
  fboss/agent/platforms/sai/SaiBcmWedge40Platform.cpp
  fboss/agent/platforms/sai/SaiFakePlatform.cpp
  fboss/agent/platforms/sai/SaiHwPlatform.cpp
  fboss/agent/platforms/sai/SaiPlatformPort.cpp
  fboss/agent/platforms/sai/SaiPlatformInit.cpp
  fboss/agent/platforms/sai/SaiWedge400CPlatform.cpp
  fboss/agent/platforms/sai/SaiWedge400CPort.cpp
)

target_link_libraries(sai_platform
  handler
  product_info
  sai_switch
  thrift_handler
  switch_asics
  hw_switch_warmboot_helper
)

set_target_properties(sai_platform PROPERTIES COMPILE_FLAGS
  "-DSAI_VER_MAJOR=${SAI_VER_MAJOR} \
  -DSAI_VER_MINOR=${SAI_VER_MINOR}  \
  -DSAI_VER_RELEASE=${SAI_VER_RELEASE}"
)
