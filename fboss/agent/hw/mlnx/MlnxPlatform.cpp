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
#include "fboss/agent/hw/mlnx/MlnxPlatform.h"
#include "fboss/agent/hw/mlnx/MlnxPlatformPort.h"
#include "fboss/agent/hw/mlnx/MlnxInitHelper.h"
#include "fboss/agent/hw/mlnx/MlnxError.h"
#include "fboss/agent/hw/mlnx/MlnxSwitchSpectrum.h"

#include "fboss/agent/state/Port.h"
#include "fboss/agent/SwSwitch.h"

DEFINE_string(volatile_state_dir, "/tmp/fboss/volatile",
              "Directory for storing volatile state");
DEFINE_string(persistent_state_dir, "/var/fboss/persistent",
              "Directory for storing persistent state");

// Path to the xml configuration file that describes port configuration
DEFINE_string(mlnx_config, "", "Mellanox config file");
// Set local mac address for device and swid0_eth if
DEFINE_string(local_mac, "", "Local device mac address");

using std::make_unique;
using std::unique_ptr;

namespace facebook { namespace fboss {

MlnxPlatform::MlnxPlatform() :
  qsfpCache_(make_unique<QsfpCache>()){
}

MlnxPlatform::~MlnxPlatform() {
}

void MlnxPlatform::init() {
  const auto& initHelper = MlnxInitHelper::get();
  hw_ = std::move(initHelper->getMlnxSwitchInstance(this));
  std::string mlnxConfigPath;
  if(FLAGS_mlnx_config != "") {
    mlnxConfigPath = FLAGS_mlnx_config;
  } else {
    // try to get from environmental variable
    auto path = std::getenv("MLNX_CONFIG");
    if (path) {
      mlnxConfigPath = path;
    } else {
      throw FbossError("Running without hardware configuration");
    }
  }

  LOG(INFO) << "Mellanox HW configuration xml file: " << mlnxConfigPath;

  MlnxInitHelper::get()->loadConfig(mlnxConfigPath, FLAGS_local_mac);

  initLocalMac();

  dumpCommandLineFlagsToFile(getCmdLineFlagsFile());
}

void MlnxPlatform::initLocalMac() {
 const auto& config = MlnxInitHelper::get()->getConfig();
 localMac_ = folly::MacAddress(config.device.deviceMacAddress);
}

void MlnxPlatform::dumpCommandLineFlagsToFile(const std::string& filePath) {
  // To sync logging gflags params
  // between fboss agent and sx_sdk/sx_acl_rm.
  // Dump flags to temporary file, sdk_logger shared object
  // will then read temporary file and set those flags to gflags
  // This is the simplest way to get SDK logging options
  // and FBOSS logging options in sync

  std::vector<gflags::CommandLineFlagInfo> flags;
  gflags::GetAllFlags(&flags);
  std::ofstream flagsFile(filePath, std::ofstream::trunc);
  for (const auto& flag: flags) {
    if (!flag.is_default) {
      flagsFile << "--" << flag.name << "=" << flag.current_value << std::endl;
    }
  }
}

MlnxPlatform::InitPortMap MlnxPlatform::initPorts() {
  InitPortMap ports;
  // Get ports found on the hardware
  // and generate MlnxPlatformPort objects
  auto mlnxPorts = hw_->getMlnxPortsAttr();

  for(const auto& portAttr: mlnxPorts) {
    std::vector<int32_t> channels {};
    auto local_port = portAttr.port_mapping.local_port;
    auto portId = PortID(local_port);
    auto transId = folly::Optional<TransceiverID>(
      static_cast<TransceiverID>(portAttr.port_mapping.module_port));

    auto lanesBmap = portAttr.port_mapping.lane_bmap;
    for (int lane = 0; lanesBmap; ++lane, lanesBmap >>= 1) {
      if (lanesBmap & 0x1) {
        channels.push_back(lane);
      }
    }

    auto mlnxPltfPort = make_unique<MlnxPlatformPort>(this,
      portId,
      transId,
      channels);

    ports.emplace(portAttr.log_port, mlnxPltfPort.get());
    ports_.emplace(portId, std::move(mlnxPltfPort));
  }
  return ports;
}

MlnxPlatformPort* MlnxPlatform::getPort(PortID portId) const {
  auto platformPortIt = ports_.find(portId);
  if (platformPortIt == ports_.end()) {
    throw FbossError("No such port with port ID ", portId);
  }
  return platformPortIt->second.get();
}

PlatformPort* MlnxPlatform::getPlatformPort(PortID portId) const {
  auto platformPortIt = ports_.find(portId);
  if (platformPortIt == ports_.end()) {
    throw FbossError("No such port with port ID ", portId);
  }
  return platformPortIt->second.get();
}

HwSwitch* MlnxPlatform::getHwSwitch() const {
  return hw_.get();
}

void MlnxPlatform::onHwInitialized(SwSwitch* sw) {
  qsfpCache_->init(sw->getQsfpCacheEvb());
  sw->registerStateObserver(this, "MlnxPlatform");
}

void MlnxPlatform::stateUpdated(const StateDelta& delta) {
  QsfpCache::PortMapThrift changedPorts;
  auto portsDelta = delta.getPortsDelta();
  for (const auto& entry : portsDelta) {
    auto newPort = entry.getNew();
    if (newPort) {
      auto platformPort = getPort(newPort->getID());
      if (platformPort->supportsTransceiver()) {
        changedPorts[newPort->getID()] = platformPort->toThrift(newPort);
      }
    }
  }
  qsfpCache_->portsChanged(changedPorts);
}

folly::MacAddress MlnxPlatform::getLocalMac() const {
  return localMac_;
}

unique_ptr<ThriftHandler> MlnxPlatform::createHandler(SwSwitch* sw) {
  return std::make_unique<ThriftHandler>(sw);
}

std::string MlnxPlatform::getCmdLineFlagsFile() {
  constexpr auto kPathSeperator = "/";
  return getVolatileStateDir() + kPathSeperator + "fboss_cmd_flags";
}

std::string MlnxPlatform::getVolatileStateDir() const {
  return FLAGS_volatile_state_dir;
}

std::string MlnxPlatform::getPersistentStateDir() const {
  return FLAGS_persistent_state_dir;
}

void MlnxPlatform::getProductInfo(ProductInfo& /*info*/) {
  // TODO(stepanb): fill in product info for mellanox platform
}

TransceiverIdxThrift MlnxPlatform::getPortMapping(PortID portId) const {
  return getPort(portId)->getTransceiverMapping();
}

QsfpCache* MlnxPlatform::getQsfpCache() const {
  return qsfpCache_.get();
}

}} // facebook::fboss
