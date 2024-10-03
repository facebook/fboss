# CMake to build libraries and binaries in fboss/agent/platforms/common

# In general, libraries and binaries in fboss/foo/bar are built by
# cmake/FooBar.cmake

add_library(platform_mapping
  fboss/agent/platforms/common/MultiPimPlatformMapping.cpp
  fboss/agent/platforms/common/PlatformMapping.cpp
)

target_link_libraries(platform_mapping
  agent_features
  error
  fboss_config_utils
  platform_config_cpp2
  state
  ${RE2}
)

add_library(platform_mapping_utils
  fboss/agent/platforms/common/PlatformMappingUtils.cpp
)

target_link_libraries(platform_mapping_utils
  error
  minipack_platform_mapping
  elbert_platform_mapping
  yamp_platform_mapping
  fake_test_platform_mapping
  fuji_platform_mapping
  galaxy_platform_mapping
  wedge100_platform_mapping
  wedge40_platform_mapping
  wedge400_platform_utils
  wedge400c_platform_utils
  darwin_platform_mapping
  wedge400_platform_mapping
  wedge400c_platform_mapping
  morgan_platform_mapping
  cloud_ripper_platform_mapping
  meru400biu_platform_mapping
  meru400bfu_platform_mapping
  montblanc_platform_mapping
  meru400bia_platform_mapping
  meru800bia_platform_mapping
  meru800bfa_platform_mapping
  janga800bic_platform_mapping
  tahan800bc_platform_mapping
  ${RE2}
)
