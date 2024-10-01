cmake_minimum_required(VERSION 3.8)

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

set(SAI_VER_MAJOR "1")
set(SAI_VER_MINOR "14")
set(SAI_VER_RELEASE "0")

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
find_package(FBThrift CONFIG REQUIRED)
include_directories(${FB303_INCLUDE_DIR})
find_path(FATAL_INCLUDE_DIR NAMES fatal/portability.h)
include_directories(${FATAL_INCLUDE_DIR})


find_library(NLOHMANN_JSON nlohmann-json)
find_path(NLOHMANN_JSON_INCLUDE_DIRS NAMES nlohmann/json.hpp)
include_directories(${NLOHMANN_JSON_INCLUDE_DIRS})

# (TODO): Pinning python version to 3.8 to compile in lego-linux since
# python 3.10 is causing link failures for sai test binary. Undo
# this once lego-linux is compatible with python3.10
find_package(Python3 3.8 EXACT COMPONENTS Interpreter Development)
include_directories(${Python3_INCLUDE_DIR})

find_path(SAI_INCLUDE_DIR NAMES sai.h)
include_directories(${SAI_INCLUDE_DIR})

if (SAI_BRCM_IMPL)
  find_path(SAI_EXPERIMENTAL_INCLUDE_DIR NAMES saiswitchextensions.h)
  include_directories(${SAI_EXPERIMENTAL_INCLUDE_DIR})
  message(STATUS, "Experimental dir: ${SAI_EXPERIMENTAL_INCLUDE_DIR}")
else()
  find_path(SAI_EXPERIMENTAL_INCLUDE_DIR
    NAMES
    experimental/saiswitchextensions.h)
  include_directories(${SAI_EXPERIMENTAL_INCLUDE_DIR}/experimental)
  message(STATUS, "Experimental Dir: ${SAI_EXPERIMENTAL_INCLUDE_DIR}")
endif()

message(STATUS "Found SAI_INCLUDE_DIR: ${SAI_INCLUDE_DIR}")

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

# Generate thrift deps and libraries
include(FBThriftCppLibrary)

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
add_fbthrift_cpp_library(
  network_address_cpp2
  common/network/if/Address.thrift
  OPTIONS
    json
    reflection
)
add_fbthrift_cpp_library(
  mpls_cpp2
  fboss/agent/if/mpls.thrift
  OPTIONS
    json
    reflection
)
add_fbthrift_cpp_library(
  fboss_common_cpp2
  fboss/lib/if/fboss_common.thrift
  OPTIONS
    json
    reflection
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
add_fbthrift_cpp_library(
  link_cpp2
  fboss/lib/phy/link.thrift
  OPTIONS
    json
    reflection
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
add_fbthrift_cpp_library(
  asic_config_cpp2
  fboss/agent/hw/sai/config/asic_config.thrift
  OPTIONS
    json
    reflection
)
add_fbthrift_cpp_library(
  asic_config_v2_cpp2
  fboss/agent/hw/config/asic_config_v2.thrift
  OPTIONS
    json
    reflection
)
add_fbthrift_cpp_library(
  mka_structs_cpp2
  fboss/mka_service/if/mka_structs.thrift
  OPTIONS
    json
    reflection
)
add_fbthrift_cpp_library(
  agent_stats_cpp2
  fboss/agent/agent_stats.thrift
  OPTIONS
    json
    reflection
  DEPENDS
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
    phy_cpp2
    prbs_cpp2
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

add_custom_target(fboss_platform_services)
add_dependencies(fboss_platform_services
  sensor_service
  sensor_service_hw_test
  sensor_service_sw_test
  fan_service
  fan_service_sw_test
  fan_service_hw_test
  rackmon
  rackmon_test
  platform_manager
  fw_util
  weutil
  led_service
  data_corral_service
)

add_custom_target(qsfp_targets)
add_dependencies(qsfp_targets
  qsfp_service
  qsfp_hw_test
  wedge_qsfp_util
)

add_custom_target(fboss_other_services)
add_dependencies(fboss_other_services
  qsfp_targets
  fboss2
  fsdb
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
  hw_ctrl_cpp2
  sai_ctrl_cpp2
  hw_test_ctrl_cpp2
  sai_test_ctrl_cpp2
  multiswitch_ctrl_cpp2
  lldp_structs_cpp2
  validated_shell_commands_cpp2
)

add_custom_target(fboss_fake_agent_targets)
add_dependencies(fboss_fake_agent_targets
  sai_test-fake
  sai_agent_hw_test-fake
  multi_switch_agent_hw_test
  fboss_agent_thrift_libs
)
if (BUILD_SAI_FAKE_LINK_TEST)
add_dependencies(fboss_fake_agent_targets
  sai_link_test-fake
)
endif()
endif()

if (GITHUB_ACTIONS_BUILD)
  add_custom_target(github_actions_fboss_platform_services)
  add_dependencies(github_actions_fboss_platform_services
    # Services
    sensor_service
    fan_service
    rackmon
    platform_manager
    fw_util
    weutil
    led_service
    data_corral_service
    # Unit Tests
    platform_manager_device_path_resolver_test
    platform_manager_platform_explorer_test
    platform_data_corral_sw_test
    platform_manager_utils_test
    platform_helpers_platform_name_lib_test
    platform_manager_data_store_test
    platform_manager_config_validator_test
    platform_manager_presence_checker_test
    platform_manager_i2c_explorer_test
    weutil_crc16_ccitt_test
    weutil_fboss_eeprom_parser_test
    sensor_service_sw_test
    sensor_service_utils_tests
    rackmon_test
    fan_service_sw_test
    data_corral_service_hw_test
    fan_service_hw_test
    sensor_service_hw_test
    weutil_hw_test
  )
endif()
