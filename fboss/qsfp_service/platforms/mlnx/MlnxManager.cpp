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
#include "fboss/qsfp_service/platforms/mlnx/MlnxManager.h"
#include "fboss/qsfp_service/platforms/mlnx/MlnxQsfp.h"
#include "fboss/agent/hw/mlnx/MlnxError.h"

#include <boost/property_tree/xml_parser.hpp>
#include <boost/property_tree/ptree.hpp>
#include <boost/foreach.hpp>

#include <folly/gen/Base.h>
#include <folly/String.h>
#include <folly/Format.h>

#include "fboss/qsfp_service/sff/QsfpModule.h"

extern "C" {
#include <sx/sxd/sxd_access_register_init.h>
#include <sx/sxd/sxd_dpt.h>
#include <complib/sx_log.h>
}

DEFINE_int32(sxd_verbosity, 1,
  "Verbosity level passed to sxd_access_reg_init");
// Path to the xml configuration file that describes port configuration
DEFINE_string(mlnx_config, "", "Mellanox config file");

extern void loggingCallback(sx_log_severity_t severity,
  const char* moduleName, char* msg);

namespace facebook { namespace fboss {

MlnxManager::MlnxManager() {
  sxd_status_t rc;

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

  // need only to get all <module> values from xml
  std::ifstream config(mlnxConfigPath);

  using boost::property_tree::ptree;

  ptree pt;
  read_xml(config, pt);
  auto portList = pt.get_child("root.device.ports-list");
  for(const auto& v: portList) {
    auto module = v.second.get<uint8_t>("frontpanel-port");
    modules_.insert(module);
  }

  auto sxdVerbosity = static_cast<sx_verbosity_level_t>(FLAGS_sxd_verbosity);
  CHECK(SX_VERBOSITY_LEVEL_CHECK_RANGE(sxdVerbosity)) << "Invalid verbosity level passed";

  pid_t pid = getpid();
  rc = sxd_access_reg_init(pid, loggingCallback, sxdVerbosity);
  mlnxCheckSxdFail(rc, "sxd_access_reg_init",
      "Failed to initialize access to registers. "
      "This could happen because sxdkernel is not running or device is not initialized. "
      "Start FBOSS agent first");
}

MlnxManager::~MlnxManager() {
  sxd_status_t rc;

  // Deinit registers access
  rc = sxd_access_reg_deinit();
  if (rc != SXD_STATUS_SUCCESS) {
    LOG(ERROR) << "sxd_access_reg_deinit(); "
               << "Failed to deinit registers access: "
               << (SXD_STATUS_MSG(rc));
  }
}

void MlnxManager::initTransceiverMap() {
  for (auto module: modules_) {
    auto qsfpImpl = std::make_unique<MlnxQsfp>(module);
    auto qsfp = std::make_unique<QsfpModule>(std::move(qsfpImpl));
    qsfp->refresh();
    transceivers_.push_back(std::move(qsfp));
  }
}

void MlnxManager::getTransceiversInfo(std::map<int32_t, TransceiverInfo>& info,
    std::unique_ptr<std::vector<int32_t>> ids) {
  LOG(INFO) << "Received request for getTransceiverInfo, with ids: " <<
    (ids->size() > 0 ? folly::join(",", *ids) : "None");
  if (ids->empty()) {
    folly::gen::range(0, getNumQsfpModules()) |
      folly::gen::appendTo(*ids);
  }

  for (const auto& i : *ids) {
    TransceiverInfo transceiverInfo;
    if (isValidTransceiver(i)) {
      transceiverInfo = transceivers_[TransceiverID(i)]->getTransceiverInfo();
    }
    info[i] = transceiverInfo;
  }
}

void MlnxManager::getTransceiversRawDOMData(
    std::map<int32_t, RawDOMData>& info,
    std::unique_ptr<std::vector<int32_t>> ids) {
  LOG(INFO) << "Received request for getTransceiversRawDOMData, with ids: " <<
    (ids->size() > 0 ? folly::join(",", *ids) : "None");
  if (ids->empty()) {
    folly::gen::range(0, getNumQsfpModules()) |
      folly::gen::appendTo(*ids);
  }
  for (const auto& i : *ids) {
    RawDOMData data;
    if (isValidTransceiver(i)) {
      data = transceivers_[TransceiverID(i)]->getRawDOMData();
    }
    info[i] = data;
  }
}

void MlnxManager::customizeTransceiver(int32_t idx, cfg::PortSpeed speed) {
  DLOG(INFO) << __FUNCTION__;

  transceivers_.at(idx)->customizeTransceiver(speed);
}

void MlnxManager::syncPorts(
    std::map<int32_t, TransceiverInfo>& info,
    std::unique_ptr<std::map<int32_t, PortStatus>> ports) {
  DLOG(INFO) << __FUNCTION__;

  // generate vector of groups of ports that share the same transceiver.
  // In case of splitted ports there could be possibly 4 ports with
  // same transceiver id
  auto groups = folly::gen::from(*ports)
    | folly::gen::filter([](const std::pair<int32_t, PortStatus>& item) {
        return item.second.__isset.transceiverIdx;
      })
    | folly::gen::groupBy([](const std::pair<int32_t, PortStatus>& item) {
        return item.second.transceiverIdx.transceiverId;
      })
    | folly::gen::as<std::vector>();

  for (auto& group : groups) {
    int32_t transceiverIdx = group.key();
    LOG(INFO) << "Syncing ports of transceiver " << transceiverIdx;

    auto transceiver = transceivers_.at(transceiverIdx).get();
    transceiver->transceiverPortsChanged(group.values());
    info[transceiverIdx] = transceiver->getTransceiverInfo();
  }
}

int MlnxManager::getNumQsfpModules() {
  DLOG(INFO) << __FUNCTION__;

  return modules_.size();
}

void MlnxManager::refreshTransceivers() {
  DLOG(INFO) << __FUNCTION__;

  for (const auto& transceiver : transceivers_) {
    transceiver->refresh();
  }
}

}} // facebook::fboss
