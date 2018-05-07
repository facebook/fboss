/* 
 * Copyright (c) Mellanox Technologies. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. Neither the names of the copyright holders nor the names of its
 *    contributors may be used to endorse or promote products derived from
 *    this software without specific prior written permission.
 *
 * Alternatively, this software may be distributed under the terms of the
 * GNU General Public License ("GPL") version 2 as published by the Free
 * Software Foundation.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */
#include "fboss/agent/hw/mlnx/MlnxInitHelper.h"
#include "fboss/agent/hw/mlnx/MlnxPCIProfiles.h"
#include "fboss/agent/hw/mlnx/MlnxDevice.h"
#include "fboss/agent/hw/mlnx/MlnxSwitchSpectrum.h"
#include "fboss/agent/hw/mlnx/MlnxError.h"

#include "fboss/agent/FbossError.h"

extern "C" {
#include <sx/sdk/sx_api_init.h>
#include <sx/sdk/sx_api_topo.h>
#include <sx/sdk/sx_api_port.h>
#include <sx/sdk/sx_api_fdb.h>
#include <sx/sdk/sx_api_host_ifc.h>
#include <sx/sdk/sx_api_fdb.h>
#include <sx/sdk/sx_api_vlan.h>
#include <sx/sdk/sx_api_router.h>
#include <sx/sxd/sxd_access_register.h>
#include <sx/sxd/sxd_command_ifc.h>
#include <sx/sxd/sxd_dpt.h>
#include <sx/sxd/sxd_status.h>
#include <resource_manager/resource_manager.h>
}

#include <unistd.h>

#include <folly/MacAddress.h>
#include <gflags/gflags.h>

#include <folly/Format.h>

#include <cstdlib>

DEFINE_string(sdk_logger, "libmlnx_logger.so",
  "Shared object with log_cb callback to pass to SDK process");

// SDK per module verbosity levels
// TODO: acl and cos module will be added
//       once we will implement support for acl and cos
DEFINE_string(verbosity_sdk_port_module,
  "error",
  "SDK port module verbosity level");
DEFINE_validator(verbosity_sdk_port_module,
  &facebook::fboss::MlnxInitHelper::validateVerbosityLevel);

DEFINE_string(verbosity_sdk_host_ifc_module,
  "error",
  "SDK host interface module verbosity level");
DEFINE_validator(verbosity_sdk_host_ifc_module,
  &facebook::fboss::MlnxInitHelper::validateVerbosityLevel);

DEFINE_string(verbosity_sdk_fdb_module,
  "error",
  "SDK fdb module verbosity level");
DEFINE_validator(verbosity_sdk_fdb_module,
  &facebook::fboss::MlnxInitHelper::validateVerbosityLevel);

DEFINE_string(verbosity_sdk_vlan_module,
  "error",
  "SDK vlan module verbosity level");
DEFINE_validator(verbosity_sdk_vlan_module,
  &facebook::fboss::MlnxInitHelper::validateVerbosityLevel);

DEFINE_string(verbosity_sdk_router_module,
  "error",
  "SDK router module verbosity level");
DEFINE_validator(verbosity_sdk_router_module,
  &facebook::fboss::MlnxInitHelper::validateVerbosityLevel);

DEFINE_string(verbosity_sdk_system,
  "error",
  "SDK system verbosity level");
DEFINE_validator(verbosity_sdk_system,
  &facebook::fboss::MlnxInitHelper::validateVerbosityLevel);

extern void loggingCallback(sx_log_severity_t severity, 
  const char* module, char* msg);

namespace {

// The default logging verbosity for sdk logs
constexpr auto kDefaultSdkLogVerbosity = SX_VERBOSITY_LEVEL_ERROR;
// sysfs
constexpr auto kChipInfoTypePath =
  "/sys/module/sx_core/parameters/chip_info_type";
constexpr auto kChipInfoRevPath =
  "/sys/module/sx_core/parameters/chip_info_revision";

constexpr auto kMaxVlanRouterInterfaces = 64;
constexpr auto kMaxPortRouterInterfaces = 64;
}

namespace facebook { namespace fboss {

const std::map<std::string, sx_verbosity_level_t>
MlnxInitHelper::strToLogVerbosityMap {
  {"none", SX_VERBOSITY_LEVEL_NONE},
  {"error", SX_VERBOSITY_LEVEL_ERROR},
  {"warning", SX_VERBOSITY_LEVEL_WARNING},
  {"notice", SX_VERBOSITY_LEVEL_NOTICE},
  {"info", SX_VERBOSITY_LEVEL_INFO},
  {"debug", SX_VERBOSITY_LEVEL_DEBUG},
  {"funcs", SX_VERBOSITY_LEVEL_FUNCS},
  {"frames", SX_VERBOSITY_LEVEL_FRAMES},
  {"all", SX_VERBOSITY_LEVEL_ALL},
};

bool MlnxInitHelper::validateVerbosityLevel(const char* /* flagname */,
    const std::string& verbosity) {
  auto iter = strToLogVerbosityMap.find(verbosity);
  if (iter == strToLogVerbosityMap.end()) {
    return false;
  }
  return true;
}

sx_verbosity_level_t MlnxInitHelper::strToLogVerbosity(const std::string& verbosity) {
  auto iter = strToLogVerbosityMap.find(verbosity);
  if (iter == strToLogVerbosityMap.end()) {
    throw FbossError("Invalid verbosity level for sdk ", verbosity);
  }
  return iter->second;
}

/**
 * return the pointer to singleton
 */
MlnxInitHelper* MlnxInitHelper::get() {
  static std::unique_ptr<MlnxInitHelper> _instance {new MlnxInitHelper()};
  return _instance.get();
}

MlnxInitHelper::MlnxInitHelper() {
  // this code will run at the start of agent
  // before anything else since we need sxdkernel
  // loaded to init chip type and decide which MlnxSwitch
  // implementation to use (MlnxSwitchSpectrum/MlnxSwitchSpectrum2)

  // restart sxdkernel, will reload sxdkernel in case when
  // FBOSS agent has failed and left sxdkernel loaded
  restartSxdKernel();

  // inits chip type and chip resource limits
  initChipType();

  // initialize DPT shared memory and init registers access
  // before device and sdk initialization
  initDptAndRegisterAccess();
}

/**
 * Reads the chip type from sysfs
 */
void MlnxInitHelper::initChipType() {
  sx_status_t rc;

  uint16_t deviceID;
  uint16_t deviceHwRevision;

  std::ifstream ifs;

  ifs.open(kChipInfoTypePath);
  if (ifs.is_open()) {
    ifs >> deviceID;
    LOG(INFO) << " [ Init ] Device id: " << deviceID;
    ifs.close();
  } else {
    throw FbossError("Cannot open ", kChipInfoTypePath);
  }

  ifs.open(kChipInfoRevPath);
  if (ifs.is_open()) {
    ifs >> deviceHwRevision;
    LOG(INFO) << " [ Init ] Device HW revision: " << deviceHwRevision;
    ifs.close();
  } else {
    throw FbossError("Cannot open ", kChipInfoRevPath);
  }

  switch(deviceID) {
  case SXD_MGIR_HW_DEV_ID_SPECTRUM:
    if (deviceHwRevision == 0xA0) {
      chipType_ = SX_CHIP_TYPE_SPECTRUM;
      LOG(INFO) << " [ Init ] Spectrum chip detected";
    } else if (deviceHwRevision == 0xA1) {
      LOG(INFO) << " [ Init ] Spectrum A1 chip detected";
      chipType_ = SX_CHIP_TYPE_SPECTRUM_A1;
    } else {
      throw FbossError("Unsupported Spectrum chip version, HW revision: ",
        static_cast<int>(deviceHwRevision));
    }
    break;
  case SXD_MGIR_HW_DEV_ID_SPECTRUM2:
    LOG(INFO) << " [ Init ] Spectrum 2 chip detected";
    chipType_ = SX_CHIP_TYPE_SPECTRUM2;
    throw FbossError("Spectrum 2 currently not supported");
    break;
  default:
    throw FbossError("Unsupported chip type, devID: ",
      static_cast<int>(deviceID),
      " HW revision: ",
      static_cast<int>(deviceHwRevision));
  }

  // get resource limits for this chip
  rc = rm_chip_limits_get(chipType_, &resourceLimits_);
  mlnxCheckSdkFail(rc,
    "rm_chip_limits_get",
    "Failed to get chip limits for chip ",
    SX_CHIP_TYPE_STR(chipType_));
}

void MlnxInitHelper::initDptAndRegisterAccess() {
  sxd_status_t rc;

  rc = sxd_dpt_init(SYS_TYPE_EN, loggingCallback, kDefaultSdkLogVerbosity);
  mlnxCheckSxdFail(rc, "sxd_dpt_init", "Failed to init DPT shared memory");

  rc = sxd_access_reg_init(0, loggingCallback, kDefaultSdkLogVerbosity);
  mlnxCheckSxdFail(rc, "sxd_access_reg_init", "Failed to init register access");
}

void MlnxInitHelper::deinitDptAndRegisterAccess() {
  sx_status_t rc;

  rc = sxd_status_to_sx_status(sxd_access_reg_deinit());
  mlnxLogSxError(rc,
    "sxd_access_reg_deinit",
    "Failed to deinit registers access");

  rc = sxd_status_to_sx_status(sxd_dpt_deinit());
  mlnxLogSxError(rc, "sxd_dpt_deinit");
}

std::unique_ptr<MlnxDevice> MlnxInitHelper::getSingleDevice(MlnxSwitch* hw) {
  int ret;
  const int maxSupportedDevices = 1;

  uint32_t devNum = maxSupportedDevices;
  std::array<char, MAX_NAME_LEN> deviceName;
  std::array<char*, maxSupportedDevices> deviceNames {deviceName.data()};

  // get the device list
  ret = sxd_get_dev_list(static_cast<char**>(deviceNames.data()), &devNum);

  // ret > 0 more devices than expected
  if (ret > 0) {
    throw FbossError("sxd_get_dev_list() returned ", devNum,
      " devices... Supported ",
      maxSupportedDevices);
  }

  mlnxCheckFail(ret, "sxd_get_dev_list", "Failed to get device list");

  sxd_dev_id_t devId = mlnxConfig_.device.deviceId;

  configureSingleDevice(devId, deviceName);

  // create MlnxDevice object
  return std::make_unique<MlnxDevice>(hw, mlnxConfig_.device);

}

void MlnxInitHelper::configureSingleDevice(sxd_dev_id_t devId,
  std::array<char, MAX_NAME_LEN>& deviceName) {
  int ret;
  sxd_status_t rc;

  sxd_ctrl_pack_t ctrl_pack {};
  ku_dpt_path_add path {};
  ku_dpt_path_modify path_modify {};
  ku_swid_details swid_details {};
  sxd_handle sxd_handle {};

  sx_pci_profile profile;

  switch(getChipType()) {
  case SX_CHIP_TYPE_SPECTRUM:
  case SX_CHIP_TYPE_SPECTRUM_A1:
    profile = kPciProfileSingleEthSpectrum;
    break;
  case SX_CHIP_TYPE_SPECTRUM2:
    throw FbossError("Spectrum 2 currently not supported");
    break;
  default:
    throw FbossError("Unsupported profile for this chip");
  }

  rc = sxd_dpt_set_access_control(devId, READ_WRITE);
  mlnxCheckSxdFail(rc,
    "sxd_dpt_set_access_control",
    "Failed to set dpt access control");

  // open device handle
  ret = sxd_open_device(deviceName.data(), &sxd_handle);
  mlnxCheckFail(ret, "sxd_open_device", "Failed to open device");

  // add I2C path
  ctrl_pack.ctrl_cmd = CTRL_CMD_ADD_DEV_PATH;
  ctrl_pack.cmd_body = static_cast<void*>(&path);
  path.dev_id = devId;
  path.path_type = DPT_PATH_I2C;
  path.path_info.sx_i2c_info.sx_i2c_dev = 0x0;
  ret = sxd_ioctl(sxd_handle, &ctrl_pack);
  mlnxCheckFail(ret, "sxd_ioctl", "Failed to add I2C dev path to DP table");

  // add PCI path
  ctrl_pack.ctrl_cmd = CTRL_CMD_ADD_DEV_PATH;
  ctrl_pack.cmd_body = static_cast<void*>(&path);
  path.dev_id = devId;
  path.path_type = DPT_PATH_PCI_E;
  path.path_info.sx_pcie_info.pci_id = 256;
  path.is_local = 1;
  ret = sxd_ioctl(sxd_handle, &ctrl_pack);
  mlnxCheckFail(ret, "sxd_ioctl", "Failed to add PCI dev path to DP table");

  // set command ifc, mad, emad, cr paths to PCI

  ctrl_pack.ctrl_cmd = CTRL_CMD_SET_CMD_PATH;
  ctrl_pack.cmd_body = static_cast<void*>(&path_modify);
  path_modify.dev_id = devId;
  path_modify.path_type = DPT_PATH_PCI_E;
  ret = sxd_ioctl(sxd_handle, &ctrl_pack);
  mlnxCheckFail(ret,
    "sxd_ioctl",
    "Failed to set cmd_ifc path in DP table to PCI");

  ctrl_pack.ctrl_cmd = CTRL_CMD_SET_EMAD_PATH;
  ret = sxd_ioctl(sxd_handle, &ctrl_pack);
  mlnxCheckFail(ret,
    "sxd_ioctl",
    "Failed to set emad path in DP table to PCI");

  ctrl_pack.ctrl_cmd = CTRL_CMD_SET_MAD_PATH;
  ret = sxd_ioctl(sxd_handle, &ctrl_pack);
  mlnxCheckFail(ret,
    "sxd_ioctl",
    "Failed to set mad access path in DP table to PCI");

  ctrl_pack.ctrl_cmd = CTRL_CMD_SET_CR_ACCESS_PATH;
  ret = sxd_ioctl(sxd_handle, &ctrl_pack);
  mlnxCheckFail(ret,
    "sxd_ioctl",
    "Failed to set cr access path in DP table to PCI");

  // reset ASIC
  ctrl_pack.ctrl_cmd = CTRL_CMD_RESET;
  ctrl_pack.cmd_body = (void*)(1);
  ret = sxd_ioctl(sxd_handle, &ctrl_pack);
  mlnxCheckFail(ret, "sxd_ioctl", "failed to reset asic");

  // set PCI profile
  profile.dev_id = devId;
  ctrl_pack.ctrl_cmd = CTRL_CMD_SET_PCI_PROFILE;
  ctrl_pack.cmd_body = static_cast<void*>(&profile);
  ret = sxd_ioctl(sxd_handle, &ctrl_pack);
  mlnxCheckFail(ret, "sxd_ioctl", "Failed to set PCI profile in ASIC");

  auto swid = getDefaultSwid();
  folly::MacAddress devMac {mlnxConfig_.device.deviceMacAddress};

  // enable device's swid
  swid_details.dev_id = devId;
  ctrl_pack.cmd_body = (void*)(&swid_details);
  ctrl_pack.ctrl_cmd = CTRL_CMD_ENABLE_SWID;
  swid_details.swid = swid;
  swid_details.iptrap_synd = SXD_TRAP_ID_IPTRAP_MIN + swid;
  swid_details.mac = devMac.u64HBO();
  ret = sxd_ioctl(sxd_handle, &ctrl_pack);
  mlnxCheckFail(ret,
    "sxd_ioctl",
    "Failed to enable swid ",
    swid);

  // close device handle
  ret = sxd_close_device(sxd_handle);
  mlnxCheckFail(ret, "sxd_close_device", "Failed to close the device");
}

void MlnxInitHelper::restartSxdKernel() {
  LOG(INFO) << " [ Init ] Starting sxdkernel...";

  auto devNull = open("/dev/null", O_WRONLY);
  if (devNull < 0) {
    throw FbossError("Cannot open /dev/null");
  }

  folly::Subprocess sxdkernelCommand(std::vector<std::string>{
      "/etc/init.d/sxdkernel",
      "restart"
    },
    folly::Subprocess::Options()
      .stdoutFd(dup(devNull))
      .stderrFd(dup(devNull)));

  auto rc = sxdkernelCommand.wait();

  close(devNull);

  mlnxCheckFail(rc.exitStatus(),
    "Failed to start sxdkernel",
    rc.str());
}

void MlnxInitHelper::startSdkProcess() {
  int ret;
  auto sdk_logger = FLAGS_sdk_logger.c_str();
  // remove /tmp/sdk_ready if it was created
  // by previous sx_sdk start
  if (access("/tmp/sdk_ready", F_OK) == 0) {
    ret = remove("/tmp/sdk_ready");
    if (ret == 0) {
        LOG(INFO) << " [ Init ] /tmp/sdk_ready removed";
    } else {
        LOG(ERROR) << " [ Init ] unable to remove /tmp/sdk_ready";
    }
  }

  auto devNull = open("/dev/null", O_WRONLY);
  if (devNull < 0) {
    throw FbossError("Cannot open /dev/null");
  }

  sdkProcess_ = folly::Subprocess(std::vector<std::string> {
      "/usr/bin/sx_sdk",
      "--logger",
      sdk_logger
    },
    folly::Subprocess::Options().
      stdoutFd(dup(devNull)).
      processGroupLeader());

  aclRmProcess_ = folly::Subprocess(std::vector<std::string> {
      "/usr/bin/sx_acl_rm",
      "--logger",
      sdk_logger
    },
    folly::Subprocess::Options().
      stdoutFd(dup(devNull)).
      processGroupLeader());

  close(devNull);

  LOG(INFO) << " [ Init ] SDK process has been started";
}

void MlnxInitHelper::initSdk() {
  sx_status_t rc;
  sx_api_sx_sdk_init_t sdkInitParams;
  std::memset(&sdkInitParams, 0, sizeof(sdkInitParams));
  uint32_t bridge_acls;
  std::memset(&bridge_acls, 0, sizeof(bridge_acls));
  uint8_t port_phy_bits_num;
  uint8_t port_pth_bits_num;
  uint8_t port_sub_bits_num;

  // start the SDK process
  startSdkProcess();

  // wait for sdk to be ready
  waitForSdk();

  // Open handle
  rc = sx_api_open(loggingCallback, &handle_);
  mlnxCheckSdkFail(rc,
    "sx_api_open",
    "Can't open communication channel to SDK");

  sdkInitParams.app_id = htonl(*((uint32_t*)"SDK1"));

  sdkInitParams.policer_params.priority_group_num = 3;

  sdkInitParams.port_params.max_dev_id = SX_DEV_ID_MAX;

  sdkInitParams.topo_params.max_num_of_tree_per_chip = 18;

  sdkInitParams.lag_params.max_ports_per_lag = 0;
  sdkInitParams.lag_params.default_lag_hash  = SX_LAG_DEFAULT_LAG_HASH;

  sdkInitParams.vlan_params.def_vid = SX_VLAN_DEFAULT_VID;
  sdkInitParams.vlan_params.max_swid_id = SX_SWID_ID_MAX;

  sdkInitParams.fdb_params.max_mc_group = SX_FDB_MAX_MC_GROUPS;
  sdkInitParams.fdb_params.flood_mode = FLOOD_PER_VLAN;

  sdkInitParams.mstp_params.mode = SX_MSTP_MODE_RSTP;

  sdkInitParams.router_profile_params.min_router_counters = 16;

  sdkInitParams.acl_params.max_swid_id = 0;

  auto& flowCounterParamsRef = sdkInitParams.flow_counter_params;
  flowCounterParamsRef.flow_counter_byte_type_min_number = 0;
  flowCounterParamsRef.flow_counter_packet_type_min_number = 0;
  flowCounterParamsRef.flow_counter_byte_type_max_number = 1600;
  flowCounterParamsRef.flow_counter_packet_type_max_number = 1600;

  sdkInitParams.acl_params.max_acl_ingress_groups = 200;
  sdkInitParams.acl_params.max_acl_egress_groups = 200;

  sdkInitParams.acl_params.min_acl_rules = 0;
  sdkInitParams.acl_params.max_acl_rules = 1600;
  sdkInitParams.acl_params.acl_search_type = SX_API_ACL_SEARCH_TYPE_PARALLEL;

  auto& bridgeInitParamRef = sdkInitParams.bridge_init_params;
  bridgeInitParamRef.sdk_mode = SX_MODE_HYBRID;
  bridgeInitParamRef.sdk_mode_params.mode_1D.max_bridge_num = 512;
  bridgeInitParamRef.sdk_mode_params.mode_1D.max_virtual_ports_num = 512;
  bridgeInitParamRef.sdk_mode_params.mode_1D.multiple_vlan_bridge_mode =
    SX_BRIDGE_MULTIPLE_VLAN_MODE_HOMOGENOUS;
  // correct the min/max acls according to bridge requirments
  // number for homgenous mode egress rules
  bridge_acls = bridgeInitParamRef.sdk_mode_params.mode_1D.max_bridge_num;
  // number for ingress rules
  bridge_acls +=
    bridgeInitParamRef.sdk_mode_params.mode_1D.max_virtual_ports_num;

  sdkInitParams.acl_params.max_acl_rules += bridge_acls;

  port_phy_bits_num = SX_PORT_UCR_ID_PHY_NUM_OF_BITS;
  port_pth_bits_num = 16;
  port_sub_bits_num = 0;

  sdkInitParams.port_params.port_phy_bits_num = port_phy_bits_num;
  sdkInitParams.port_params.port_pth_bits_num = port_pth_bits_num;
  sdkInitParams.port_params.port_sub_bits_num = port_sub_bits_num;

  bridgeInitParamRef.sdk_mode_params.mode_hybrid.max_bridge_num = 512;

  sdkInitParams.profile = kSinglePartEthDeviceProfileSpectrum;
  sdkInitParams.profile.dev_id = 1;
  sdkInitParams.profile.chip_type = (sxd_chip_types)chipType_;

  sdkInitParams.pci_profile = kPciProfileSingleEthSpectrum;
  sdkInitParams.pci_profile.dev_id = 1;

  sdkInitParams.applibs_mask = SX_API_FLOW_COUNTER | SX_API_POLICER |
    SX_API_HOST_IFC | SX_API_SPAN |
    SX_API_ETH_L2 | SX_API_ACL;

  rc = sx_api_sdk_init_set(handle_, &sdkInitParams);
  mlnxCheckSdkFail(rc, "sx_api_sdk_init_set", "Failed to initialize SDK");

  setVerbosityLevels();
}

void MlnxInitHelper::setVerbosityLevels() {
  sx_status_t rc {SX_STATUS_ERROR};
  // cast integers to verbosity level, no check here
  // sx_api_*_log_verbosity_level_set does range check for verbosity leve
  auto system_verbosity_level = strToLogVerbosity(FLAGS_verbosity_sdk_system);
  auto port_verbosity_level = strToLogVerbosity(FLAGS_verbosity_sdk_port_module);
  auto host_ifc_verbosity_level = strToLogVerbosity(FLAGS_verbosity_sdk_host_ifc_module);
  auto fdb_verbosity_level = strToLogVerbosity(FLAGS_verbosity_sdk_fdb_module);
  auto vlan_verbosity_level = strToLogVerbosity(FLAGS_verbosity_sdk_vlan_module);
  auto router_verbosity_level = strToLogVerbosity(FLAGS_verbosity_sdk_router_module);

  // first set global system verbosity level and the per each module
  rc = sx_api_system_log_verbosity_level_set(handle_,
    SX_LOG_VERBOSITY_TARGET_MODULE,
    system_verbosity_level,
    system_verbosity_level);
  mlnxCheckSdkFail(rc,
    "sx_api_system_log_verbosity_level_set",
    "Failed to set system log verbosity to ",
    SX_VERBOSITY_LEVEL_STR(system_verbosity_level));

  LOG(INFO) << "System verbosity level set to "
            << (SX_VERBOSITY_LEVEL_STR(system_verbosity_level));

  // port module
  rc = sx_api_port_log_verbosity_level_set(handle_,
    SX_LOG_VERBOSITY_TARGET_MODULE,
    port_verbosity_level,
    port_verbosity_level);
  mlnxCheckSdkFail(rc,
    "sx_api_port_log_verbosity_level_set",
    "Failed to set port log verbosity to ",
    SX_VERBOSITY_LEVEL_STR(port_verbosity_level));

  LOG(INFO) << "Port module verbosity level set to "
            << (SX_VERBOSITY_LEVEL_STR(port_verbosity_level));

  // fdb module
  rc = sx_api_fdb_log_verbosity_level_set(handle_,
    SX_LOG_VERBOSITY_TARGET_MODULE,
    fdb_verbosity_level,
    fdb_verbosity_level);
  mlnxCheckSdkFail(rc,
    "sx_api_fdb_log_verbosity_level_set",
    "Failed to set fdb log verbosity to ",
    SX_VERBOSITY_LEVEL_STR(fdb_verbosity_level));

  LOG(INFO) << "Fdb module verbosity level set to "
            << (SX_VERBOSITY_LEVEL_STR(fdb_verbosity_level));

  // vlan module
  rc = sx_api_vlan_log_verbosity_level_set(handle_,
    SX_LOG_VERBOSITY_TARGET_MODULE,
    vlan_verbosity_level,
    vlan_verbosity_level);
  mlnxCheckSdkFail(rc,
    "sx_api_vlan_log_verbosity_level_set",
    "Failed to set vlan log verbosity to ",
    SX_VERBOSITY_LEVEL_STR(vlan_verbosity_level));

  LOG(INFO) << "Vlan module verbosity level set to "
            << (SX_VERBOSITY_LEVEL_STR(vlan_verbosity_level));

  // host interface module
  rc = sx_api_host_ifc_log_verbosity_level_set(handle_,
    SX_LOG_VERBOSITY_TARGET_MODULE,
    host_ifc_verbosity_level,
    host_ifc_verbosity_level);
  mlnxCheckSdkFail(rc,
    "sx_api_host_ifc_log_verbosity_level_set",
    "Failed to set host interface log verbosity to ",
    SX_VERBOSITY_LEVEL_STR(host_ifc_verbosity_level));

  LOG(INFO) << "Host interface module verbosity level set to "
            << (SX_VERBOSITY_LEVEL_STR(host_ifc_verbosity_level));

  // router module
  rc = sx_api_router_log_verbosity_level_set(handle_,
    SX_LOG_VERBOSITY_TARGET_MODULE,
    router_verbosity_level,
    router_verbosity_level);
  mlnxCheckSdkFail(rc,
    "sx_api_router_log_verbosity_level_set",
    "Failed to set router log verbosity to ",
    SX_VERBOSITY_LEVEL_STR(router_verbosity_level));

  LOG(INFO) << "Router module verbosity level set to "
            << (SX_VERBOSITY_LEVEL_STR(router_verbosity_level));
}

void MlnxInitHelper::loadConfig(const std::string& pathToConfigFile,
  const std::string& localMac) {
  try {
    mlnxConfig_.parseConfigXml(pathToConfigFile);
  } catch(std::exception& e) {
    throw FbossError("Error while parsing mellanox configuration file:",
      e.what());
  }
  if (localMac != "") {
    mlnxConfig_.device.deviceMacAddress = localMac;
  }
  configLoaded_.store(true, std::memory_order_release);
}

void MlnxInitHelper::init() {
  if(!configLoaded_.load(std::memory_order_acquire)) {
    throw FbossError("Running without hardware configuration");
  }

  initImpl();
}

void MlnxInitHelper::waitForSdk() {
  const unsigned kTimeUnit = 100;
  const unsigned kWaitSdkTimeout = 100000; // 10 sec
  unsigned int iteration = 0;
  LOG(INFO) << " [ Init ] Waiting until SDK becomes ready...";
  // wait until /tmp/sdk_ready gets created
  while (0 != access("/tmp/sdk_ready", F_OK)) {
    // sleep for 100 useconds
    usleep(kTimeUnit);
    ++iteration;
    if (iteration >= kWaitSdkTimeout) {
       throw FbossError("Timeout for waiting until SDK gets ready expired");
    }
  }
  LOG(INFO) << " [ Init ] SDK ready after "
            << kTimeUnit*iteration << " usecs";
}

void MlnxInitHelper::initImpl() {
  initSdk();
  enableSwids();
}

void MlnxInitHelper::enableSwids() {
  sx_status_t rc;
  rc = sx_api_port_swid_set(handle_,
    SX_ACCESS_CMD_ADD,
    getDefaultSwid());
  mlnxCheckSdkFail(rc,
    "sx_api_port_swid_set",
    "Cannot enable swid ",
    getDefaultSwid());
  LOG(INFO) << " [ Init ] Swid "
            << getDefaultSwid()
            << " has been enabled";
}

std::unique_ptr<MlnxSwitch> MlnxInitHelper::getMlnxSwitchInstance(
  MlnxPlatform* pltf) {
  switch(getChipType()) {
  case SX_CHIP_TYPE_SPECTRUM:
  case SX_CHIP_TYPE_SPECTRUM_A1:
    return std::make_unique<MlnxSwitchSpectrum>(pltf);
    break;
  default:
    throw FbossError("Unsupported chip type", SX_CHIP_TYPE_STR(chipType_));
    break;
  }
}

void MlnxInitHelper::initRouter() {
  sx_status_t rc;
  CHECK(handle_) << "Invalid SDK handle";
  sx_router_general_param_t generalParams;
  sx_router_resources_param_t rsrcParams;

  // IPv4/v6 unicast enabled, multicast disabled
  generalParams.ipv4_enable = SX_ROUTER_ENABLE_STATE_ENABLE;
  generalParams.ipv6_enable = SX_ROUTER_ENABLE_STATE_ENABLE;
  generalParams.ipv4_mc_enable = SX_ROUTER_ENABLE_STATE_DISABLE;
  generalParams.ipv6_mc_enable = SX_ROUTER_ENABLE_STATE_DISABLE;
  generalParams.rpf_enable = SX_ROUTER_ENABLE_STATE_DISABLE;

  // router resources, the min* fields form sx_router_resources_param_t
  // are set to 0
  memset(&rsrcParams, 0, sizeof(rsrcParams));
  const auto& rsrcCfg = mlnxConfig_.routerRsrc;

  rsrcParams.max_virtual_routers_num = rsrcCfg.maxVrfNum;
  rsrcParams.max_vlan_router_interfaces = rsrcCfg.maxVlanRouterInterfaces;
  rsrcParams.max_port_router_interfaces = rsrcCfg.maxPortRouterInterfaces;
  rsrcParams.max_router_interfaces = rsrcCfg.maxRouterInterfaces;
  rsrcParams.max_ipv4_neighbor_entries = rsrcCfg.maxV4NeighEntries;
  rsrcParams.max_ipv6_neighbor_entries = rsrcCfg.maxV6NeighEntries;
  rsrcParams.max_ipv4_uc_route_entries = rsrcCfg.maxV4RouteEntries;
  rsrcParams.max_ipv6_uc_route_entries = rsrcCfg.maxV6RouteEntries;

  auto checkLimitOrSetDefault =
    [&] (auto& param, const char*  name, auto limit, auto defaultValue) {
      if (param == 0) {
        param = defaultValue;
      } else if (param > limit) {
        throw FbossError("Param: ", name, ", value: ", param,
          " - exceeded resource limit: ", limit);
      }
  };

  // set 0 values to limits max
  checkLimitOrSetDefault(rsrcParams.max_virtual_routers_num, "max-vrf",
      resourceLimits_.router_vrid_max, resourceLimits_.router_vrid_max);
  checkLimitOrSetDefault(rsrcParams.max_vlan_router_interfaces,
      "max-vlan-router-interfaces",
      kMaxVlanRouterInterfaces, kMaxVlanRouterInterfaces);
  checkLimitOrSetDefault(rsrcParams.max_port_router_interfaces,
      "max-port-router-interfaces",
      kMaxPortRouterInterfaces, kMaxPortRouterInterfaces);
  checkLimitOrSetDefault(rsrcParams.max_ipv4_neighbor_entries,
      "max-ipv4-neighbor-entries",
      resourceLimits_.router_neigh_max, 0);
  checkLimitOrSetDefault(rsrcParams.max_ipv6_neighbor_entries,
      "max-ipv6-neighbor-entries",
      resourceLimits_.router_neigh_max, 0);
  checkLimitOrSetDefault(rsrcParams.max_ipv4_uc_route_entries,
      "max-ipv4-route-entries",
      resourceLimits_.router_ipv4_uc_max, 0);
  checkLimitOrSetDefault(rsrcParams.max_ipv6_uc_route_entries,
      "max-ipv6-route-entries",
      resourceLimits_.router_ipv6_uc_max, 0);

  rc = sx_api_router_init_set(handle_, &generalParams, &rsrcParams);
  mlnxCheckSdkFail(rc,
    "sx_api_router_init_set",
    "Failed to init router module");
  routerInitialized_ = true;

  LOG(INFO) << " [ Init ] Router initialized";
}

void MlnxInitHelper::stopSdk() {
  sdkProcess_.sendSignal(SIGINT);
  aclRmProcess_.sendSignal(SIGINT);

  auto interruptAndWaitForExit = [] (auto& process) {
    process.sendSignal(SIGINT);
    try {
      process.waitChecked();
    } catch (const std::exception& err) {
      LOG(ERROR) << "Failed to interrupt process " << err.what();
    }
  };

  // interrupt sdk
  interruptAndWaitForExit(sdkProcess_);
  interruptAndWaitForExit(aclRmProcess_);
}

void MlnxInitHelper::deinitRouter() {
  sx_status_t rc;
  CHECK(handle_) << "Invalid SDK handle";
  if (!routerInitialized_) {
    return;
  }
  rc = sx_api_router_deinit_set(handle_);
  mlnxLogSxError(rc,
    "sx_api_router_deinit_set",
    "Failed to deinit router module");
  LOG_IF(INFO, rc == SX_STATUS_SUCCESS) << " [ Exit ] Router deinitialized";
}

void MlnxInitHelper::deinit() {
  sx_status_t rc;
  if (handle_ != SX_API_INVALID_HANDLE) {
    rc = sx_api_close(&handle_);
    mlnxLogSxError(rc,
      "sx_api_close",
      "Failed to close SDK communication channel");
  }
  deinitDptAndRegisterAccess();
  stopSdk();
  stopSxdKernel();
}

void MlnxInitHelper::stopSxdKernel() {
  int ret;
  LOG(INFO) << " [ Exit ] Stopping sxdkernel...";

  auto devNull = open("/dev/null", O_WRONLY);
  if (devNull < 0) {
    throw FbossError("Cannot open /dev/null");
  }

  folly::Subprocess sxdkernelCommand(std::vector<std::string>{
      "/etc/init.d/sxdkernel",
      "stop"
    },
    folly::Subprocess::Options()
      .stdoutFd(dup(devNull))
      .stderrFd(dup(devNull)));

  auto rc = sxdkernelCommand.wait();

  close(devNull);

  if (rc.exitStatus() != EXIT_SUCCESS) {
    LOG(ERROR) << "Failed to start sxdkernel: "
               << rc.str();
  }
}

sx_api_handle_t MlnxInitHelper::getHandle() const {
  return handle_;
}

sx_swid_t MlnxInitHelper::getDefaultSwid() const {
  return kDefaultSwid_;
}

sx_chip_types_t MlnxInitHelper::getChipType() const {
  return chipType_;
}

const rm_resources_t& MlnxInitHelper::getRmLimits() const {
  return resourceLimits_;
}

MlnxConfig& MlnxInitHelper::getConfig() {
  return mlnxConfig_;
}

}} // facebook::fboss
