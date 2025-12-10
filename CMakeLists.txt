cmake_minimum_required(VERSION 3.8)

include(functions.cmake)

option(WITH_ASAN "Turn ON address sanitizer build options" OFF)
if ($ENV{WITH_ASAN})
  message(STATUS "ENV WITH_ASAN is set")
  set (WITH_ASAN ON)
endif()

if (WITH_ASAN)
  message(STATUS "Building WITH_ASAN")
endif()

option(BENCHMARK_INSTALL "Installation of benchmark test binaries" OFF)
if ($ENV{BENCHMARK_INSTALL})
  message(STATUS "ENV BENCHMARK_INSTALL is set")
  set (BENCHMARK_INSTALL ON)
endif()

if (BENCHMARK_INSTALL)
  message(STATUS "Configured to install benchmark binaries")
endif()

option(BUILD_SAI_FAKE "Build sai binaries for fake_sai" OFF)
if ($ENV{BUILD_SAI_FAKE})
  message(STATUS "ENV BUILD_SAI_FAKE is set")
  set (BUILD_SAI_FAKE ON)
endif()

option(GITHUB_ACTIONS_BUILD
  "Enables custom targets specific to GitHub Actions automation"
  OFF)
if ($ENV{GITHUB_ACTIONS_BUILD})
  message(STATUS "ENV GITHUB_ACTIONS_BUILD is set")
  set (GITHUB_ACTIONS_BUILD ON)
endif()

option(BUILD_SAI_FAKE_BENCHMARKS "Build benchmarks binaries for fake_sai" OFF)
if ($ENV{BUILD_SAI_FAKE_BENCHMARKS})
  message(STATUS "ENV BUILD_SAI_FAKE_BENCHMARKS is set")
  set (BUILD_SAI_FAKE_BENCHMARKS ON)
endif()

if (BUILD_SAI_FAKE_BENCHMARKS)
  message(STATUS "Building benchmarks binaries for fake_sai")
endif()

option(SKIP_ALL_INSTALL "Installation of fboss binaries" OFF)
if ($ENV{SKIP_ALL_INSTALL})
  message(STATUS "ENV SKIP_ALL_INSTALL is set")
  set (SKIP_ALL_INSTALL ON)
endif()

if (SKIP_ALL_INSTALL)
  install(FILES README.md DESTINATION .)
  macro (install)
    message(STATUS "Skipping installing ${ARGV} fboss artifact")
    _install(${ARGV} EXCLUDE_FROM_ALL)
  endmacro ()
endif()

option(SAI_TAJO_IMPL "Build SAI api with tajo extensions" OFF)
if ($ENV{SAI_TAJO_IMPL})
  message(STATUS "ENV SAI_TAJO_IMPL is set")
  set (SAI_TAJO_IMPL ON)
endif()

option(SAI_BRCM_IMPL "Build SAI api with BRCM-SAI extensions" OFF)
if ($ENV{SAI_BRCM_IMPL})
  message(STATUS "ENV SAI_BRCM_IMPL is set")
  set (SAI_BRCM_IMPL ON)
endif()

option(CHENAB_SAI_SDK "Build SAI api with CHENAB-SAI extensions" OFF)
if ($ENV{CHENAB_SAI_SDK})
  message(STATUS "ENV CHENAB_SAI_SDK is set")
  set (CHENAB_SAI_SDK ON)
  link_directories(/var/FBOSS/sai_impl/lib)
  link_directories(/var/FBOSS/sdk/usr/lib64)
  add_definitions (-DCHENAB_SAI_SDK)
endif()

option(SAI_BRCM_PAI_IMPL "Build SAI api with BRCOM_PAI-SAI extensions" OFF)
if ($ENV{SAI_BRCM_PAI_IMPL})
  message(STATUS "ENV SAI_BRCM_PAI_IMPL is set")
  set(SAI_BRCM_PAI_IMPL ON)
  # NOTE: Unlike agent that we use sai_impl, we directly use pre-built SDK from
  # local directory
  # Header files for libpai.a, the whole inc directory from SDK
  set(SAI_BRCM_PAI_IMPL_INC_DIR "/var/FBOSS/pai_impl/include")
  # macsec headers
  include_directories("${SAI_BRCM_PAI_IMPL_INC_DIR}/pai_macsec")

  # SDK artifacts directory which hosts libpai.a, EPDM SW artifact, PHY Driver
  # SW artifact
  set(SAI_BRCM_PAI_IMPL_LIB_DIR "/var/FBOSS/pai_impl/lib")
  set(CMAKE_PREFIX_PATH ${SAI_BRCM_PAI_IMPL_LIB_DIR} ${CMAKE_PREFIX_PATH})
  find_library(PAI_LIB pai)
  find_library(EPDM_LIB epdm)
  find_library(DRIVER_LIB phymodepil)
  message(
    STATUS
    "PAI_LIB: ${PAI_LIB}, EPDM_LIB: ${EPDM_LIB}, DRIVER_LIB: ${DRIVER_LIB}"
  )
  set(XPHY_SDK_LIBS
    ${DRIVER_LIB}
    ${EPDM_LIB}
    ${PAI_LIB}
  )
endif()

if (NOT $ENV{SAI_VERSION} EQUAL "")
  message(STATUS "ENV SAI_VERSION is set")
  set (SAI_VERSION $ENV{SAI_VERSION})
else()
  message(STATUS "ENV SAI_VERSION is not set, using default:1.16.3")
  set (SAI_VERSION "1.16.3")
endif()

option(BUILD_SAI_FAKE_LINK_TEST "Building of SAI Fake Link Test binaries" OFF)
if ($ENV{BUILD_SAI_FAKE_LINK_TEST})
  message(STATUS "ENV BUILD_SAI_FAKE_LINK_TEST is set")
  set (BUILD_SAI_FAKE_LINK_TEST ON)
endif()

if (BUILD_SAI_FAKE_LINK_TEST)
  message(STATUS "Configured to build SAI Fake Link Test binaries")
endif()

if (SAI_BRCM_IMPL)
  # TODO (skhare) IS_OSS_BRCM_SAI is only a short-term change to unblock.
  # Refactor/fix the code and then remove this flag.
  add_definitions (-DIS_OSS_BRCM_SAI=true)
  # Build for BCM DNX by default. For XGS, set the ENV variable SAI_SDK_VERSION
  # to build for BCM XGS.
  set(SAI_SDK_VERSION "SAI_VERSION_11_0_EA_DNX_ODP")
endif()

# SAI version is set. Extract MAJOR, MINOR, RELEASE ver
strtok(${SAI_VERSION} "." tokens)
set(listLength 0)
list(LENGTH tokens listLength)
if(NOT ${listLength} EQUAL 3)
  message(Invalid SAI_VERSION in env variable $ENV{SAI_VERSION})
else()
  list(GET tokens 0 SAI_VER_MAJOR)
  list(GET tokens 1 SAI_VER_MINOR)
  list(GET tokens 2 SAI_VER_RELEASE)
endif()
#endif

if (NOT "$ENV{SAI_SDK_VERSION}" STREQUAL "")
  message(STATUS "ENV SAI_SDK_VERSION is set to $ENV{SAI_SDK_VERSION}")
  set(SAI_SDK_VERSION "$ENV{SAI_SDK_VERSION}")
endif()
message(STATUS "Building for SAI_SDK_VERSION: ${SAI_SDK_VERSION}")

if (NOT "${SAI_SDK_VERSION}" STREQUAL "")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -D${SAI_SDK_VERSION}")
endif()
message(STATUS "CMAKE_CXX_FLAGS: ${CMAKE_CXX_FLAGS}")

message(STATUS "Building for SAI Spec ${SAI_VER_MAJOR}.${SAI_VER_MINOR}.${SAI_VER_RELEASE}")

set(CMAKE_MODULE_PATH
  "${CMAKE_CURRENT_SOURCE_DIR}/build/fbcode_builder/CMake"
  ${CMAKE_MODULE_PATH})

set (CMAKE_MODULE_PATH
  "${CMAKE_MODULE_PATH};${CMAKE_CURRENT_SOURCE_DIR}/cmake")

if (POLICY CMP0054)
  # Enable CMP0054 to indicate that we want the new safer if() argument behavior
  cmake_policy(SET CMP0054 NEW)
endif()
project(FBOSS)

include(CMakeParseArguments)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Werror")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-sign-compare")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-maybe-uninitialized")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wno-deprecated-declarations")
if (WITH_ASAN)
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O0 -g -static-libasan")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-sanitize-recover=all")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-omit-frame-pointer")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-common")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize-address-use-after-scope")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=undefined")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-sanitize=alignment")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-sanitize=null")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-sanitize=vptr")
  set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static-libasan")
endif()

include_directories(${CMAKE_SOURCE_DIR})
include_directories(${CMAKE_BUILD_DIR})

find_package(GTest REQUIRED)
include_directories(
  ${GTEST_INCLUDE_DIRS}
)

find_package(GMock MODULE REQUIRED)
find_package(gflags REQUIRED)
include_directories(${GFLAGS_INCLUDE_DIR})
find_package(Glog REQUIRED)
include_directories(${GLOG_INCLUDE_DIRS})
find_package(folly CONFIG REQUIRED)
include_directories(${FOLLY_INCLUDE_DIR})
find_package(Boost REQUIRED)
include_directories(${Boost_INCLUDE_DIR})
find_package(fb303 CONFIG REQUIRED)
find_package(fizz CONFIG REQUIRED)
find_package(fmt CONFIG REQUIRED)
find_package(wangle CONFIG REQUIRED)
find_package(FBThrift CONFIG REQUIRED COMPONENTS)
find_package(range-v3 REQUIRED)
include_directories(${FB303_INCLUDE_DIR})
find_path(FATAL_INCLUDE_DIR NAMES fatal/portability.h)
include_directories(${FATAL_INCLUDE_DIR})


find_library(NLOHMANN_JSON nlohmann-json)
find_path(NLOHMANN_JSON_INCLUDE_DIRS NAMES nlohmann/json.hpp)
include_directories(${NLOHMANN_JSON_INCLUDE_DIRS})
find_package(Python3 COMPONENTS Interpreter Development)
include_directories(${Python3_INCLUDE_DIR})

# SAI_BRCM_PAI_IMPL SDK comes with SAI headers instead of using public SAI
if (SAI_BRCM_PAI_IMPL)
  set(SAI_INCLUDE_DIR "${SAI_BRCM_PAI_IMPL_INC_DIR}/sai/inc")
else()
  find_path(SAI_INCLUDE_DIR NAMES sai.h)
endif()
include_directories(${SAI_INCLUDE_DIR})
message(STATUS "Found SAI Include dir: ${SAI_INCLUDE_DIR}")

if (SAI_BRCM_IMPL)
  find_path(SAI_EXPERIMENTAL_INCLUDE_DIR NAMES saiswitchextensions.h)
  include_directories(${SAI_EXPERIMENTAL_INCLUDE_DIR})
  message(STATUS "Found SAI Experimental dir: ${SAI_EXPERIMENTAL_INCLUDE_DIR}")
elseif(CHENAB_SAI_SDK)
  # Chenab-TODO: set experimental directories appropriately
  set(SAI_EXPERIMENTAL_INCLUDE_DIR "/var/FBOSS/sai_impl_output/include")
  include_directories(${SAI_EXPERIMENTAL_INCLUDE_DIR})
  message(STATUS "Found SAI Experimental dir: ${SAI_EXPERIMENTAL_INCLUDE_DIR}")
elseif(SAI_BRCM_PAI_IMPL)
  set(
    SAI_EXPERIMENTAL_INCLUDE_DIR
    "${SAI_BRCM_PAI_IMPL_INC_DIR}/sai/experimental"
  )
  include_directories(${SAI_EXPERIMENTAL_INCLUDE_DIR})
  message(STATUS "Found SAI Experimental dir: ${SAI_EXPERIMENTAL_INCLUDE_DIR}")
else()
  find_path(SAI_EXPERIMENTAL_INCLUDE_DIR
    NAMES
    experimental/saiswitchextensions.h)
  include_directories(${SAI_EXPERIMENTAL_INCLUDE_DIR}/experimental)
  message(
    STATUS
    "Found SAI Experimental dir: ${SAI_EXPERIMENTAL_INCLUDE_DIR}/experimental"
  )
endif()

find_library(LIBNL nl-3)
find_path(LIBNL_INCLUDE_DIR NAMES libnl3/netlink/socket.h)
include_directories(${LIBNL_INCLUDE_DIR})
include_directories(${LIBNL_INCLUDE_DIR}/libnl3)

find_library(IPROUTE2 netlink)
find_path(IPROUTE2_INCLUDE_DIR NAMES libnetlink.h)
include_directories(${IPROUTE2_INCLUDE_DIR})

find_library(YAML-CPP yaml-cpp)
find_path(YAML-CPP_INCLUDE_DIR NAMES yaml-cpp/yaml.h)
include_directories(${YAML-CPP_INCLUDE_DIR})
message(STATUS "Found YAML-CPP_INCLUDE_DIR: ${YAML-CPP_INCLUDE_DIR}")

find_library(YAML yaml)
find_path(YAML_INCLUDE_DIR NAMES yaml.h)
include_directories(${YAML_INCLUDE_DIR})
message(STATUS "Found YAML_INCLUDE_DIR: ${YAML_INCLUDE_DIR}")

find_package(python-pyyaml REQUIRED)

# Generate thrift deps and libraries
include(FBThriftCppLibrary)
include(FBThriftPyLibrary)

# Thrift libraries
add_fbthrift_cpp_library(
  fb303_cpp2
  common/fb303/if/fb303.thrift
  SERVICES
    FacebookService
  OPTIONS
    json
    reflection
)
add_fbthrift_py_library(
  fb303_py
  common/fb303/if/fb303.thrift
  SERVICES
    FacebookService
  OPTIONS
    json
)
add_fbthrift_cpp_library(
  network_address_cpp2
  common/network/if/Address.thrift
  OPTIONS
    json
    reflection
)
add_fbthrift_py_library(
  network_address_py
  common/network/if/Address.thrift
  NAMESPACE "facebook.network.Address"
  OPTIONS
    json
)
add_fbthrift_cpp_library(
  mpls_cpp2
  fboss/agent/if/mpls.thrift
  OPTIONS
    json
    reflection
)
add_fbthrift_py_library(
  mpls_py
  fboss/agent/if/mpls.thrift
  NAMESPACE "neteng.fboss.mpls"
  OPTIONS
    json
)
add_fbthrift_cpp_library(
  fboss_common_cpp2
  fboss/lib/if/fboss_common.thrift
  OPTIONS
    json
    reflection
)
add_fbthrift_py_library(
  fboss_common_py
  fboss/lib/if/fboss_common.thrift
  NAMESPACE "neteng.fboss.fboss_common"
  OPTIONS
    json
)
add_fbthrift_cpp_library(
  switch_config_cpp2
  fboss/agent/switch_config.thrift
  OPTIONS
    json
    reflection
  DEPENDS
    common_cpp2
    fboss_common_cpp2
    mpls_cpp2
)
add_fbthrift_py_library(
  switch_config_py
  fboss/agent/switch_config.thrift
  NAMESPACE "neteng.fboss.switch_config"
  OPTIONS
    json
  DEPENDS
    common_py
    fboss_common_py
    mpls_py
)
add_fbthrift_cpp_library(
  agent_config_cpp2
  fboss/agent/agent_config.thrift
  OPTIONS
    json
    reflection
  DEPENDS
    platform_config_cpp2
    switch_config_cpp2
)
add_fbthrift_cpp_library(
  agent_info_cpp2
  fboss/agent/agent_info.thrift
  OPTIONS
    json
    reflection
)
add_fbthrift_cpp_library(
  platform_config_cpp2
  fboss/agent/platform_config.thrift
  OPTIONS
    json
    reflection
  DEPENDS
    bcm_config_cpp2
    asic_config_cpp2
    asic_config_v2_cpp2
    phy_cpp2
)
add_fbthrift_py_library(
  platform_config_py
  fboss/agent/platform_config.thrift
  NAMESPACE "neteng.fboss.platform_config"
  OPTIONS
    json
  DEPENDS
    bcm_config_py
    asic_config_py
    asic_config_v2_py
    phy_py
)
add_fbthrift_cpp_library(
  switch_reachability_cpp2
  fboss/agent/switch_reachability.thrift
  OPTIONS
    json
    reflection
)
add_fbthrift_cpp_library(
  switch_state_cpp2
  fboss/agent/switch_state.thrift
  OPTIONS
    json
    reflection
  DEPENDS
    switch_config_cpp2
    phy_cpp2
    ctrl_cpp2
)
add_fbthrift_cpp_library(
  prbs_cpp2
  fboss/lib/phy/prbs.thrift
  OPTIONS
    json
    reflection
)
add_fbthrift_py_library(
  prbs_py
  fboss/lib/phy/prbs.thrift
  NAMESPACE "neteng.fboss.prbs"
  OPTIONS
    json
)
add_fbthrift_cpp_library(
  link_cpp2
  fboss/lib/phy/link.thrift
  OPTIONS
    json
    reflection
)
add_fbthrift_py_library(
  link_py
  fboss/lib/phy/link.thrift
  NAMESPACE "neteng.fboss.link"
  OPTIONS
    json
)
add_fbthrift_cpp_library(
  transceiver_cpp2
  fboss/qsfp_service/if/transceiver.thrift
  OPTIONS
    json
    reflection
  DEPENDS
    link_cpp2
    prbs_cpp2
)
add_fbthrift_py_library(
  transceiver_py
  fboss/qsfp_service/if/transceiver.thrift
  NAMESPACE "neteng.fboss.transceiver"
  OPTIONS
    json
  DEPENDS
    link_py
    prbs_py
)
add_fbthrift_cpp_library(
  pim_state_cpp2
  fboss/lib/if/pim_state.thrift
  OPTIONS
    json
    reflection
)
add_fbthrift_cpp_library(
  transceiver_validation_cpp2
  fboss/qsfp_service/if/transceiver_validation.thrift
  OPTIONS
    json
    reflection
  DEPENDS
    transceiver_cpp2
    switch_config_cpp2
    fboss_common_cpp2
)
add_fbthrift_cpp_library(
  qsfp_config_cpp2
  fboss/qsfp_service/if/qsfp_service_config.thrift
  OPTIONS
    json
    reflection
  DEPENDS
    transceiver_cpp2
    transceiver_validation_cpp2
    switch_config_cpp2
)
add_fbthrift_cpp_library(
  qsfp_state_cpp2
  fboss/qsfp_service/if/qsfp_state.thrift
  OPTIONS
    json
    reflection
  DEPENDS
    qsfp_config_cpp2
    phy_cpp2
    transceiver_cpp2
    pim_state_cpp2
    port_state_cpp2
)
add_fbthrift_cpp_library(
  port_state_cpp2
  fboss/qsfp_service/if/port_state.thrift
  OPTIONS
    json
    reflection
)
add_fbthrift_cpp_library(
  qsfp_stats_cpp2
  fboss/qsfp_service/if/qsfp_stats.thrift
  OPTIONS
    json
    reflection
  DEPENDS
    hardware_stats_cpp2
    phy_cpp2
    transceiver_cpp2
)
add_fbthrift_cpp_library(
  cmis_cpp2
  fboss/qsfp_service/module/cmis/cmis.thrift
  OPTIONS
    json
    reflection
)
add_fbthrift_cpp_library(
  sff_cpp2
  fboss/qsfp_service/module/sff/sff.thrift
  OPTIONS
    json
    reflection
)
add_fbthrift_cpp_library(
  sff8472_cpp2
  fboss/qsfp_service/module/sff/sff8472.thrift
  OPTIONS
    json
    reflection
)
add_fbthrift_cpp_library(
  sensor_service_stats_cpp2
  fboss/platform/sensor_service/sensor_service_stats.thrift
  OPTIONS
    json
    reflection
  DEPENDS
    sensor_service_cpp2
)
add_fbthrift_cpp_library(
  phy_cpp2
  fboss/lib/phy/phy.thrift
  SERVICES
    FbossCommonPhyCtrl
  OPTIONS
    json
    reflection
  DEPENDS
    switch_config_cpp2
    transceiver_cpp2
    fboss_cpp2
    fb303_cpp2
    io_stats_cpp2
)
add_fbthrift_py_library(
  phy_py
  fboss/lib/phy/phy.thrift
  NAMESPACE "neteng.fboss.phy"
  OPTIONS
    json
  DEPENDS
    switch_config_py
    transceiver_py
    fboss_py
    fb303_py
    io_stats_py
)
add_fbthrift_cpp_library(
  optic_cpp2
  fboss/agent/if/optic.thrift
  OPTIONS
    json
    reflection
  DEPENDS
    transceiver_cpp2
)
add_fbthrift_cpp_library(
  fboss_cpp2
  fboss/agent/if/fboss.thrift
  OPTIONS
    json
    reflection
)
add_fbthrift_py_library(
  fboss_py
  fboss/agent/if/fboss.thrift
  NAMESPACE "neteng.fboss"
  OPTIONS
    json
)
add_fbthrift_cpp_library(
  sflow_cpp2
  fboss/agent/if/sflow.thrift
  OPTIONS
    json
)
add_fbthrift_cpp_library(
  common_cpp2
  fboss/agent/if/common.thrift
  OPTIONS
    json
    reflection
  DEPENDS
    mpls_cpp2
    network_address_cpp2
)
add_fbthrift_py_library(
  common_py
  fboss/agent/if/common.thrift
  NAMESPACE "neteng.fboss.common"
  OPTIONS
    json
  DEPENDS
    mpls_py
    network_address_py
)
add_fbthrift_cpp_library(
  product_info_cpp2
  fboss/agent/if/product_info.thrift
  OPTIONS
    json
    reflection
)
add_fbthrift_cpp_library(
  hardware_stats_cpp2
  fboss/agent/hw/hardware_stats.thrift
  OPTIONS
    json
    visitation
    reflection
  DEPENDS
    mka_structs_cpp2
)
add_fbthrift_cpp_library(
  ctrl_cpp2
  fboss/agent/if/ctrl.thrift
  SERVICES
    FbossCtrl
    NeighborListenerClient
  OPTIONS
    json
    reflection
  DEPENDS
    agent_stats_cpp2
    fboss_cpp2
    fb303_cpp2
    mpls_cpp2
    network_address_cpp2
    optic_cpp2
    platform_config_cpp2
    hardware_stats_cpp2
    product_info_cpp2
    transceiver_cpp2
    common_cpp2
    phy_cpp2
  OPTIONS
    json
)
add_fbthrift_cpp_library(
  packet_stream_cpp2
  fboss/agent/if/packet_stream.thrift
  SERVICES
    PacketStream
  DEPENDS
    fb303_cpp2
)
add_fbthrift_cpp_library(
    bcm_config_cpp2
  fboss/agent/hw/bcm/bcm_config.thrift
  OPTIONS
    json
    reflection
)
add_fbthrift_py_library(
    bcm_config_py
  fboss/agent/hw/bcm/bcm_config.thrift
  NAMESPACE "neteng.fboss.bcm_config"
  OPTIONS
    json
)
add_fbthrift_cpp_library(
  asic_config_cpp2
  fboss/agent/hw/sai/config/asic_config.thrift
  OPTIONS
    json
    reflection
)
add_fbthrift_py_library(
  asic_config_py
  fboss/agent/hw/sai/config/asic_config.thrift
  NAMESPACE "neteng.fboss.asic_config"
  OPTIONS
    json
)
add_fbthrift_cpp_library(
  asic_config_v2_cpp2
  fboss/agent/hw/config/asic_config_v2.thrift
  OPTIONS
    json
    reflection
)
add_fbthrift_py_library(
  asic_config_v2_py
  fboss/agent/hw/config/asic_config_v2.thrift
  NAMESPACE "neteng.fboss.asic_config_v2"
  OPTIONS
    json
)
add_fbthrift_cpp_library(
  mka_structs_cpp2
  fboss/mka_service/if/mka_structs.thrift
  OPTIONS
    json
    reflection
)
add_fbthrift_cpp_library(
  highfreq_cpp2
  fboss/agent/if/highfreq.thrift
  OPTIONS
    json
)
add_fbthrift_cpp_library(
  asic_temp_cpp2
  fboss/agent/if/asic_temp.thrift
  SERVICES
    AsicTempThrift
  OPTIONS
    json
    reflection
  DEPENDS
    fboss_cpp2
)
add_fbthrift_cpp_library(
  agent_stats_cpp2
  fboss/agent/agent_stats.thrift
  OPTIONS
    json
    reflection
  DEPENDS
    asic_temp_cpp2
    hardware_stats_cpp2
    phy_cpp2
)
add_fbthrift_cpp_library(
  production_features_cpp2
  fboss/agent/test/production_features.thrift
  OPTIONS
    json
    reflection
)
add_fbthrift_cpp_library(
  qsfp_production_features_cpp2
  fboss/qsfp_service/test/hw_test/qsfp_production_features.thrift
  OPTIONS
    json
    reflection
)
add_fbthrift_cpp_library(
  link_test_production_features_cpp2
  fboss/agent/test/link_tests/link_test_production_features.thrift
  OPTIONS
    json
    reflection
)
add_fbthrift_cpp_library(
  i2c_controller_stats_cpp2
  fboss/lib/i2c/i2c_controller_stats.thrift
)

add_fbthrift_cpp_library(
  io_stats_cpp2
  fboss/lib/if/io_stats.thrift
  OPTIONS
    json
    reflection
)

add_fbthrift_py_library(
  io_stats_py
  fboss/lib/if/io_stats.thrift
  NAMESPACE "neteng.fboss.io_stats"
  OPTIONS
    json
)

add_fbthrift_cpp_library(
  led_mapping_cpp2
  fboss/lib/led/led_mapping.thrift
  OPTIONS
    json
    reflection
)

add_fbthrift_cpp_library(
  bsp_platform_mapping_cpp2
  fboss/lib/bsp/bsp_platform_mapping.thrift
  OPTIONS
    json
    reflection
  DEPENDS
    led_mapping_cpp2
)

add_fbthrift_cpp_library(
  led_config_cpp2
  fboss/led_service/if/led_service_config.thrift
  OPTIONS
    json
    reflection
)

add_fbthrift_cpp_library(
  qsfp_cpp2
  fboss/qsfp_service/if/qsfp.thrift
  SERVICES
    QsfpService
  OPTIONS
    json
  DEPENDS
    fb303_cpp2
    ctrl_cpp2
    fboss_cpp2
    transceiver_cpp2
    switch_config_cpp2
    mpls_cpp2
    mka_structs_cpp2
    hardware_stats_cpp2
)

add_fbthrift_cpp_library(
  hw_ctrl_cpp2
  fboss/agent/if/hw_ctrl.thrift
  SERVICES
    FbossHwCtrl
  DEPENDS
    ctrl_cpp2
    highfreq_cpp2
    switch_state_cpp2
)

add_fbthrift_cpp_library(
  sai_ctrl_cpp2
  fboss/agent/hw/sai/switch/sai_ctrl.thrift
  SERVICES
    SaiCtrl
  DEPENDS
    ctrl_cpp2
    hw_ctrl_cpp2
)

add_fbthrift_cpp_library(
  hw_test_ctrl_cpp2
  fboss/agent/hw/hw_test_ctrl.thrift
  SERVICES
    HwTestCtrl
  DEPENDS
    ctrl_cpp2
)

add_fbthrift_cpp_library(
  sai_test_ctrl_cpp2
  fboss/agent/hw/sai/hw_test/sai_test_ctrl.thrift
  SERVICES
    SaiTestCtrl
  DEPENDS
    ctrl_cpp2
    hw_test_ctrl_cpp2
)

add_fbthrift_cpp_library(
  multiswitch_ctrl_cpp2
  fboss/agent/if/multiswitch_ctrl.thrift
  SERVICES
    MultiSwitchCtrl
  DEPENDS
    common_cpp2
    ctrl_cpp2
    fsdb_oper_cpp2
    hardware_stats_cpp2
    phy_cpp2
)

add_fbthrift_cpp_library(
  agent_hw_test_ctrl_cpp2
  fboss/agent/if/agent_hw_test_ctrl.thrift
  SERVICES
    AgentHwTestCtrl
  OPTIONS
    json
    reflection
  DEPENDS
    common_cpp2
    ctrl_cpp2
    fboss_cpp2
    switch_state_cpp2
    network_address_cpp2
    mpls_cpp2
    phy_cpp2
    prbs_cpp2
  OPTIONS
    json
)

add_fbthrift_py_library(
  platform_mapping_config_py
  fboss/lib/platform_mapping_v2/platform_mapping_config.thrift
  NAMESPACE "neteng.fboss.platform_mapping_config"
  OPTIONS
    json
)

find_library(USB usb-1.0)
find_path(USB_INCLUDE_DIR NAMES libusb-1.0/libusb.h)
include_directories(${USB_INCLUDE_DIR})

find_library(NETLINK3 nl-3)
find_library(NETLINKROUTE3 nl-route-3 libnl-route-3.a nl-route-3)

find_library(CURL curl)
find_path(CURL_INCLUDE_DIR NAMES curl/curl.h)
include_directories(${CURL_INCLUDE_DIR})

find_library(MNL mnl)
find_library(SODIUM sodium)

find_library(RE2 re2)
find_path(RE2_INCLUDE_DIR NAMES re2/re2.h)
include_directories(${RE2_INCLUDE_DIR})

find_library(EXPRTK exprtk)
find_path(EXPRTK_INCLUDE_DIR NAMES exprtk.hpp)
include_directories(${EXPRTK_INCLUDE_DIR})

find_library(LIBGPIOD gpiod)
find_path(LIBGPIOD_INCLUDE_DIR NAMES gpiod.h)
include_directories(${LIBGPIOD_INCLUDE_DIR})

find_library(SYSTEMD systemd)
find_path(SYSTEMD_INCLUDE_DIR NAMES systemd/sd-daemon.h)
include_directories(${SYSTEMD_INCLUDE_DIR})

find_package(tabulate CONFIG REQUIRED)

# Unit Testing
add_definitions (-DIS_OSS=true)
find_package(Threads REQUIRED)
enable_testing()

# Build all cmake files under cmake/*cmake
file(GLOB_RECURSE cmakeFiles ${CMAKE_CURRENT_SOURCE_DIR}/cmake/*.cmake)
foreach(cmakeFile ${cmakeFiles})
  message(status " INCLUDE ${cmakeFile}")
  INCLUDE(${cmakeFile})
endforeach(cmakeFile)

add_custom_target(fboss_agent_benchmark_libs)
add_dependencies(fboss_agent_benchmark_libs
  hw_flowlet_stats_collection_speed
  hw_rx_slow_path_arp_rate
  hw_bgp_rx_slow_path_rate
  hw_init_and_exit_benchmark_helper
  hw_system_scale_memory_benchmark
)


add_custom_target(fboss_platform_services)
add_dependencies(fboss_platform_services
  # Services
  sensor_service
  fan_service
  rackmon
  platform_manager
  fw_util
  weutil
  data_corral_service
  # Utilities
  sensor_service_client
  # Unit Tests
  platform_config_lib_config_lib_test
  platform_manager_device_path_resolver_test
  platform_manager_platform_explorer_test
  platform_data_corral_sw_test
  platform_manager_utils_test
  platform_helpers_platform_name_lib_test
  platform_helpers_platform_fs_utils_test
  platform_helpers_platform_utils_test
  platform_manager_data_store_test
  platform_manager_config_validator_test
  platform_manager_presence_checker_test
  platform_manager_i2c_explorer_test
  weutil_crc16_ccitt_test
  weutil_fboss_eeprom_interface_test
  sensor_service_sw_test
  sensor_service_utils_test
  rackmon_test
  fan_service_sw_test
  data_corral_service_hw_test
  fan_service_hw_test
  sensor_service_hw_test
  weutil_hw_test
  platform_hw_test
  platform_manager_hw_test
  cross_config_validator_test
  fw_util_hw_test
)

add_custom_target(qsfp_targets)
add_dependencies(qsfp_targets
  qsfp_service
  qsfp_hw_test
  wedge_qsfp_util
)

add_custom_target(led_targets)
add_dependencies(led_targets
  led_service
  led_service_hw_test
)

add_custom_target(fboss_other_services)
add_dependencies(fboss_other_services
  qsfp_targets
  led_targets
  fboss2
  fsdb_all_services
)

add_custom_target(fboss_platform_mapping_gen)
add_dependencies(fboss_platform_mapping_gen
  platform_mapping_gen_unit_test.GEN_PY_EXE
  platform_mapping_gen_no_regression_test.GEN_PY_EXE
)

if (BUILD_SAI_FAKE)
add_custom_target(fboss_agent_thrift_libs)
add_dependencies(fboss_agent_thrift_libs
  mpls_cpp2
  switch_config_cpp2
  agent_config_cpp2
  platform_config_cpp2
  switch_reachability_cpp2
  switch_state_cpp2
  optic_cpp2
  fboss_cpp2
  sflow_cpp2
  common_cpp2
  product_info_cpp2
  hardware_stats_cpp2
  ctrl_cpp2
  packet_stream_cpp2
  bcm_config_cpp2
  asic_config_cpp2
  asic_config_v2_cpp2
  agent_stats_cpp2
  production_features_cpp2
  link_test_production_features_cpp2
  hw_ctrl_cpp2
  sai_ctrl_cpp2
  hw_test_ctrl_cpp2
  sai_test_ctrl_cpp2
  multiswitch_ctrl_cpp2
  agent_hw_test_ctrl_cpp2
  lldp_structs_cpp2
  validated_shell_commands_cpp2
)

add_custom_target(fboss_fake_agent_targets)
add_dependencies(fboss_fake_agent_targets
  fboss_agent_thrift_libs
  fboss_sw_agent
  sai_replayer-fake
  wedge_agent-fake
  fboss_hw_agent-fake
)

add_custom_target(fboss_fake_agent_test_targets)
add_dependencies(fboss_fake_agent_test_targets
  fboss_agent_thrift_libs
  sai_test-fake
  sai_agent_hw_test-fake
  multi_switch_agent_hw_test
  sai_agent_scale_test-fake
  sai_invariant_agent_test-fake
)

if(BUILD_SAI_FAKE_BENCHMARKS)
add_custom_target(fboss_fake_agent_benchmarks)

# SAI_BENCHMARKS defined in cmake/AgentHwSaiBenchmarks.cmake
foreach(SAI_BENCHMARK IN LISTS SAI_BENCHMARKS)
    add_dependencies(fboss_fake_agent_benchmarks sai_${SAI_BENCHMARK}-fake)
endforeach()
endif()

if (BUILD_SAI_FAKE_LINK_TEST)
add_dependencies(fboss_fake_agent_test_targets
  sai_link_test-fake
  sai_mono_link_test-fake
  sai_multi_link_test-fake
)
endif()

endif()

if (GITHUB_ACTIONS_BUILD)
  add_custom_target(github_actions_fboss_other_services)
  add_dependencies(github_actions_fboss_other_services
    # Services
    fboss2
    fsdb_all_services
  )
endif()
