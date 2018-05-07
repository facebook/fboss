# Mellanox SDK libraries
find_library(SDK sxapi)
find_library(SW_RM sw_rm)
find_library(SXDEV sxdev)
find_library(SXDREGINIT sxdreg_access)

target_compile_definitions(fboss_agent PRIVATE MLNX_PLATFORM)

target_sources(fboss_agent
  PRIVATE
    fboss/agent/hw/mlnx/MlnxInitHelper.cpp
    fboss/agent/hw/mlnx/MlnxConfig.cpp
    fboss/agent/hw/mlnx/MlnxPCIProfiles.cpp
    fboss/agent/hw/mlnx/MlnxDevice.cpp
    fboss/agent/hw/mlnx/MlnxHostIfc.cpp
    fboss/agent/hw/mlnx/MlnxTxPacket.cpp
    fboss/agent/hw/mlnx/MlnxRxPacket.cpp
    fboss/agent/hw/mlnx/MlnxSwitch.cpp
    fboss/agent/hw/mlnx/MlnxSwitchSpectrum.cpp
    fboss/agent/hw/mlnx/MlnxPort.cpp
    fboss/agent/hw/mlnx/MlnxPortTable.cpp
    fboss/agent/hw/mlnx/MlnxPlatform.cpp
    fboss/agent/hw/mlnx/MlnxPlatformPort.cpp
    fboss/agent/hw/mlnx/MlnxUtils.cpp
)

add_library(mlnx_logger SHARED
  fboss/agent/hw/mlnx/MlnxSdkLogger.cpp
)

target_link_libraries(mlnx_logger
                      ${GLOG}
                      ${FOLLY}
)

target_link_libraries(fboss_agent
  PRIVATE
    mlnx_logger
    ${SDK}
    ${SW_RM}
    ${SXDREGINIT}
    ${SXDEV}
)

