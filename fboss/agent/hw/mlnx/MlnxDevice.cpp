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
#include "fboss/agent/hw/mlnx/MlnxDevice.h"
#include "fboss/agent/hw/mlnx/MlnxSwitch.h"
#include "fboss/agent/hw/mlnx/MlnxError.h"
#include "fboss/agent/hw/mlnx/MlnxUtils.h"

extern "C" {
#include <sx/sdk/sx_api_port.h>
#include <sx/sdk/sx_api_topo.h>
}

#include <folly/MacAddress.h>
#include <folly/Format.h>

namespace facebook { namespace fboss {

MlnxDevice::MlnxDevice(MlnxSwitch* hw,
    const MlnxConfig::DeviceInfo& device):
    hw_(hw),
    handle_(hw_->getHandle()),
    deviceCfg_(device),
    deviceId_(deviceCfg_.deviceId) {
  // resise vector to fit port attributes
  auto portsNumber = deviceCfg_.ports.size();
  portAttrs_.resize(portsNumber);

  // convert folly mac to sx_mac_addr_t
  utils::follyMacAddressToSdk(folly::MacAddress(deviceCfg_.deviceMacAddress),
    &baseMacAddr_);
}

MlnxDevice::~MlnxDevice() {}

void MlnxDevice::init() {
  sx_status_t rc;
  auto swid = hw_->getDefaultSwid();

  if (initialized_.load(std::memory_order_acquire)) {
    LOG(WARNING) << "Devices " << deviceId_ << " has been already initialized";
    return;
  }

  int portIndex = 0;
  for (const auto& port: deviceCfg_.ports) {
    auto& portAttributes = portAttrs_[portIndex];
    std::memset(&portAttributes, 0, sizeof(portAttributes));
    portAttributes.port_mode = SX_PORT_MODE_EXTERNAL;
    portAttributes.port_mapping.local_port = port.localPort;
    portAttributes.port_mapping.module_port = port.frontpanelPort;
    portAttributes.port_mapping.width = port.lanes.size();
    portAttributes.port_mapping.mapping_mode = SX_PORT_MAPPING_MODE_ENABLE;

    ++portIndex;
  }

  // device set. After this call we have max port width set
  // for each port
  rc = sx_api_port_device_set(handle_, SX_ACCESS_CMD_ADD,
    deviceId_, &baseMacAddr_, portAttrs_.data(), portAttrs_.size());
  mlnxCheckSdkFail(rc,
    "sx_api_port_device_set",
    "Failed to set ports for device ",
    deviceId_);

  // initialize in topology sdk lib
  sx_topolib_dev_info_t dev_info;
  dev_info.dev_id = deviceId_;
  dev_info.node_type = SX_DEV_NODE_TYPE_LEAF_LOCAL;
  dev_info.unicast_arr_len = 0;
  dev_info.unicast_tree_hndl_arr[0] = 0;

  rc = sx_api_topo_device_set(handle_, SX_ACCESS_CMD_ADD, &dev_info);
  mlnxCheckSdkFail(rc,
    "sx_api_topo_device_set",
    "Failed to add device ",
    deviceId_,
    " to topology library");

  rc = sx_api_topo_device_set(handle_, SX_ACCESS_CMD_READY, &dev_info);
  mlnxCheckSdkFail(rc,
    "sx_api_topo_device_set",
    "Failed to set ready device ",
    deviceId_,
    " to topology library");

  // apply port mapping from config
  applyPortMapping();

  // init ports
  for (const auto& portAttr: portAttrs_) {

    auto logString = folly::sformat(
      "binding and init port {:d} (logical port 0x{:x}) to swid {:d}",
      portAttr.port_mapping.local_port,
      portAttr.log_port,
      swid);

    LOG(INFO) << logString;

    rc = sx_api_port_swid_bind_set(handle_, portAttr.log_port, swid);
    mlnxCheckSdkFail(rc,
      "sx_api_port_swid_bind_set",
      "Failed to bind ",
      portAttr.log_port,
      " to swid",
       swid);

    // init port
    rc = sx_api_port_init_set(handle_, portAttr.log_port);
    mlnxCheckSdkFail(rc,
      "sx_api_port_init_set",
      "Failed to initialize ",
      portAttr.log_port,
      " in SDK");
  }

  LOG(INFO) << folly::format("device {:d} has been initialized, mac {}",
    deviceId_, folly::MacAddress(deviceCfg_.deviceMacAddress).toString());

  initialized_.store(true, std::memory_order_release);
}

void MlnxDevice::applyPortMapping() {
  sx_status_t rc;
  sx_port_log_id_t logicalPort;

  // after sx_api_port_device_set() we have
  // all @width lanes mapped for each port by default.
  // To apply actual mapping we need first to unmap
  for (const auto& port: deviceCfg_.ports) {
    logicalPort = 0;
    sx_port_mapping_t portMapping;

    std::memset(&portMapping, 0, sizeof(portMapping));

    // generate logical port
    SX_PORT_DEV_ID_SET(logicalPort, deviceId_);
    SX_PORT_TYPE_ID_SET(logicalPort, SX_PORT_TYPE_NETWORK);
    SX_PORT_PHY_ID_SET(logicalPort, port.localPort);

    portMapping.local_port = port.localPort;

    rc = sx_api_port_mapping_set(handle_, &logicalPort, &(portMapping), 1);
    mlnxCheckSdkFail(rc,
      "sx_api_port_mapping_set",
      "Failed to unmap local port - ",
      static_cast<int>(portMapping.local_port));
  }

  // port mapping
  int portIndex = 0;
  for (const auto& port: deviceCfg_.ports) {
    logicalPort = 0;
    auto& portAttr = portAttrs_[portIndex];

    // reapply port lanes bitmap
    portAttr.port_mapping.lane_bmap = 0;

    for (int laneIndex = 0; laneIndex < port.lanes.size(); ++laneIndex) {
      portAttr.port_mapping.lane_bmap |= 0x1 << port.lanes[laneIndex];
    }

    SX_PORT_DEV_ID_SET(logicalPort, deviceId_);
    SX_PORT_TYPE_ID_SET(logicalPort, SX_PORT_TYPE_NETWORK);
    SX_PORT_PHY_ID_SET(logicalPort, port.localPort);

    rc = sx_api_port_mapping_set(handle_, &logicalPort,
        &(portAttr.port_mapping), 1);
    mlnxCheckSdkFail(rc,
      "sx_api_port_mapping_set",
      "Failed to unmap local port - ",
      static_cast<int>(portAttr.port_mapping.local_port),
      " Lanes bitmap : ",
      static_cast<int>(portAttr.port_mapping.lane_bmap));

    auto portMappingLogString = folly::sformat(
      "setting for device {:d} port id {:d}, transceiver id {:d}, lanes ",
      deviceId_,
      portAttrs_[portIndex].port_mapping.local_port,
      portAttrs_[portIndex].port_mapping.module_port);

    for(int lane: port.lanes) {
      folly::toAppend(lane, " ", &portMappingLogString);
    }

    LOG(INFO) << portMappingLogString;

    portIndex++;
  }
}

void MlnxDevice::deinit() {
  sx_status_t rc;
  auto swid = hw_->getDefaultSwid();

  if (!initialized_.load(std::memory_order_acquire)) {
    LOG(WARNING) << "Devices " << deviceId_ << " was not initialized";
    return;
  }

  for (const auto& portAttr: portAttrs_) {

    auto logString = folly::sformat(
      "unbinding and deinit port {:d} (logical port 0x{:x}) from swid {:d}",
      portAttr.port_mapping.local_port,
      portAttr.log_port,
      swid);

    LOG(INFO) << logString;

    // init port
    rc = sx_api_port_deinit_set(handle_, portAttr.log_port);
    mlnxCheckSdkFail(rc,
      "sx_api_port_deinit_set",
      "Failed to deinitialize ",
      portAttr.port_mapping.local_port,
      " in SDK");

    rc = sx_api_port_swid_bind_set(handle_, portAttr.log_port,
        SX_SWID_ID_DISABLED);
    mlnxCheckSdkFail(rc,
      "sx_api_port_swid_bind_set",
      "Failed to unbind port ",
      portAttr.port_mapping.local_port,
      " to swid",
       swid);
  }

  initialized_.store(false, std::memory_order_release);
}

const std::vector<sx_port_attributes_t>& MlnxDevice::getPortAttrs() const {
  return portAttrs_;
}

}} // namespace facebook::fboss
