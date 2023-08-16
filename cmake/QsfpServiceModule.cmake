# CMake to build libraries and binaries in fboss/qsfp_service/module

# In general, libraries and binaries in fboss/foo/bar are built by
# cmake/FooBar.cmake

add_library(qsfp_module STATIC
  fboss/qsfp_service/module/QsfpHelper.cpp
  fboss/qsfp_service/module/QsfpModule.cpp
  fboss/qsfp_service/module/oss/QsfpModule.cpp
  fboss/qsfp_service/module/sff/SffFieldInfo.cpp
  fboss/qsfp_service/module/sff/SffModule.cpp
  fboss/qsfp_service/module/sff/Sff8472Module.cpp
  fboss/qsfp_service/module/sff/Sff8472FieldInfo.cpp
  fboss/qsfp_service/module/oss/SffModule.cpp
  fboss/qsfp_service/module/cmis/CmisFieldInfo.cpp
  fboss/qsfp_service/module/cmis/CmisModule.cpp
)

target_link_libraries(qsfp_module
  fboss_i2c_lib
  qsfp_config_parser
  snapshot_manager
  qsfp_lib
)
