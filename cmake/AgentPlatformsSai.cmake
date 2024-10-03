# CMake to build libraries and binaries in fboss/agent/platforms/sai

# In general, libraries and binaries in fboss/foo/bar are built by
# cmake/FooBar.cmake

add_library(sai_platform
  fboss/agent/platforms/sai/SaiPlatform.cpp
  fboss/agent/platforms/sai/SaiBcmPlatform.cpp
  fboss/agent/platforms/sai/SaiBcmPlatformPort.cpp
  fboss/agent/platforms/sai/SaiBcmMinipackPlatform.cpp
  fboss/agent/platforms/sai/SaiBcmYampPlatform.cpp
  fboss/agent/platforms/sai/SaiBcmWedge100Platform.cpp
  fboss/agent/platforms/sai/SaiBcmWedge400Platform.cpp
  fboss/agent/platforms/sai/SaiBcmWedge400PlatformPort.cpp
  fboss/agent/platforms/sai/SaiBcmDarwinPlatform.cpp
  fboss/agent/platforms/sai/SaiBcmFujiPlatform.cpp
  fboss/agent/platforms/sai/SaiElbert8DDPhyPlatformPort.cpp
  fboss/agent/platforms/sai/SaiFakePlatform.cpp
  fboss/agent/platforms/sai/SaiFakePlatformPort.cpp
  fboss/agent/platforms/sai/SaiJanga800bicPlatform.cpp
  fboss/agent/platforms/sai/SaiJanga800bicPlatformPort.cpp
  fboss/agent/platforms/sai/SaiMorgan800ccPlatform.cpp
  fboss/agent/platforms/sai/SaiMorgan800ccPlatformPort.cpp
  fboss/agent/platforms/sai/SaiPlatformPort.cpp
  fboss/agent/platforms/sai/SaiPlatformInit.cpp
  fboss/agent/platforms/sai/SaiWedge400CPlatform.cpp
  fboss/agent/platforms/sai/SaiCloudRipperPlatform.cpp
  fboss/agent/platforms/sai/SaiCloudRipperPlatformPort.cpp
  fboss/agent/platforms/sai/SaiWedge400CPlatformPort.cpp
  fboss/agent/platforms/sai/SaiTajoPlatform.cpp
  fboss/agent/platforms/sai/SaiTajoPlatformPort.cpp
  fboss/agent/platforms/sai/SaiMeru400biuPlatform.cpp
  fboss/agent/platforms/sai/SaiMeru800biaPlatform.cpp
  fboss/agent/platforms/sai/SaiMeru400biaPlatform.cpp
  fboss/agent/platforms/sai/SaiMeru400biaPlatformPort.cpp
  fboss/agent/platforms/sai/SaiMeru400bfuPlatform.cpp
  fboss/agent/platforms/sai/SaiMeru800bfaPlatform.cpp
  fboss/agent/platforms/sai/SaiBcmMontblancPlatform.cpp
  fboss/agent/platforms/sai/SaiBcmMontblancPlatformPort.cpp
  fboss/agent/platforms/sai/SaiTahan800bcPlatform.cpp
  fboss/agent/platforms/sai/SaiTahan800bcPlatformPort.cpp

  fboss/agent/platforms/sai/oss/SaiBcmMinipackPlatform.cpp
  fboss/agent/platforms/sai/oss/SaiTajoPlatform.cpp
  fboss/agent/platforms/sai/oss/SaiBcmMinipackPlatformPort.cpp
  fboss/agent/platforms/sai/oss/SaiBcmFujiPlatformPort.cpp
  fboss/agent/platforms/sai/oss/SaiBcmWedge100PlatformPort.cpp
  fboss/agent/platforms/sai/oss/SaiBcmWedge400PlatformPort.cpp
  fboss/agent/platforms/sai/oss/SaiBcmDarwinPlatform.cpp
  fboss/agent/platforms/sai/oss/SaiBcmDarwinPlatformPort.cpp
  fboss/agent/platforms/sai/oss/SaiBcmYampPlatformPort.cpp
  fboss/agent/platforms/sai/SaiBcmElbertPlatform.cpp
  fboss/agent/platforms/sai/oss/SaiBcmElbertPlatformPort.cpp
  fboss/agent/platforms/sai/oss/SaiWedge400CPlatformPort.cpp
  fboss/agent/platforms/sai/oss/SaiBcmPlatform.cpp
  fboss/agent/platforms/sai/oss/SaiMeru400biuPlatform.cpp
  fboss/agent/platforms/sai/oss/SaiMeru800biaPlatform.cpp
  fboss/agent/platforms/sai/oss/SaiMeru400biuPlatformPort.cpp
  fboss/agent/platforms/sai/oss/SaiMeru800biaPlatformPort.cpp
  fboss/agent/platforms/sai/oss/SaiMeru400biaPlatform.cpp
  fboss/agent/platforms/sai/oss/SaiMeru400bfuPlatform.cpp
  fboss/agent/platforms/sai/oss/SaiMeru800bfaPlatform.cpp
  fboss/agent/platforms/sai/oss/SaiMeru400bfuPlatformPort.cpp
  fboss/agent/platforms/sai/oss/SaiMeru800bfaPlatformPort.cpp
  fboss/agent/platforms/sai/oss/SaiMorgan800ccPlatformPort.cpp
)

target_link_libraries(sai_platform
  handler
  product_info
  sai_switch
  thrift_handler
  switch_asics
  hw_switch_warmboot_helper
  fake_test_platform_mapping
  minipack_platform_mapping
  elbert_platform_mapping
  yamp_platform_mapping
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
  wedge_led_utils
  bcm_yaml_config
  cloud_ripper_platform_mapping
  meru400biu_platform_mapping
  meru400bia_platform_mapping
  meru400bfu_platform_mapping
  meru800bia_platform_mapping
  meru800bfa_platform_mapping
  montblanc_platform_mapping
  janga800bic_platform_mapping
  tahan800bc_platform_mapping
  led_structs_types_cpp2
  led_mapping_cpp2
)

set_target_properties(sai_platform PROPERTIES COMPILE_FLAGS
  "-DSAI_VER_MAJOR=${SAI_VER_MAJOR} \
  -DSAI_VER_MINOR=${SAI_VER_MINOR}  \
  -DSAI_VER_RELEASE=${SAI_VER_RELEASE}"
)

function(BUILD_SAI_WEDGE_AGENT SAI_IMPL_NAME SAI_IMPL_ARG)

  message(STATUS "Building Sai WedgeAgent SAI_IMPL_NAME: ${SAI_IMPL_NAME} SAI_IMPL_ARG: ${SAI_IMPL_ARG}")

  add_executable(wedge_agent-${SAI_IMPL_NAME}
    fboss/agent/platforms/sai/wedge_agent.cpp
  )

  target_link_libraries(wedge_agent-${SAI_IMPL_NAME}
    -Wl,--whole-archive
    main
    monolithic_agent_initializer
    sai_platform
    sai_traced_api
    ${SAI_IMPL_ARG}
    -Wl,--no-whole-archive
    ${CMAKE_THREAD_LIBS_INIT}
  )

  if (SAI_BRCM_IMPL)
    target_link_libraries(wedge_agent-${SAI_IMPL_NAME}
      ${YAML}
    )
  endif()

  set_target_properties(wedge_agent-${SAI_IMPL_NAME}
      PROPERTIES COMPILE_FLAGS
      "-DSAI_VER_MAJOR=${SAI_VER_MAJOR} \
      -DSAI_VER_MINOR=${SAI_VER_MINOR}  \
      -DSAI_VER_RELEASE=${SAI_VER_RELEASE}"
    )

  add_executable(fboss_hw_agent-${SAI_IMPL_NAME}
    fboss/agent/platforms/sai/WedgeHwAgent.cpp
    fboss/agent/platforms/sai/oss/WedgeHwAgent.cpp
  )

  target_link_libraries(fboss_hw_agent-${SAI_IMPL_NAME}
    -Wl,--whole-archive
    hwagent-main
    fboss_common_init
    load_agent_config
    sai_platform
    hwagent
    ${SAI_IMPL_ARG}
    -Wl,--no-whole-archive
  )

endfunction()

# If libsai_impl is provided, build wedge_agent linking with it
find_library(SAI_IMPL sai_impl)
message(STATUS "SAI_IMPL: ${SAI_IMPL}")

if (SAI_BRCM_IMPL)
  find_path(SAI_EXPERIMENTAL_INCLUDE_DIR NAMES saiswitchextensions.h)
  include_directories(${SAI_EXPERIMENTAL_INCLUDE_DIR})
  message(STATUS, "SAI_EXPERIMENTAL_INCLUDE_DIR: ${SAI_EXPERIMENTAL_INCLUDE_DIR}")
endif()

if(SAI_IMPL)
  BUILD_SAI_WEDGE_AGENT("sai_impl" ${SAI_IMPL})
  install(
    TARGETS
    wedge_agent-sai_impl)
  install(
    TARGETS
    fboss_hw_agent-sai_impl)
endif()
