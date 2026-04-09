/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/cli/fboss2/session/FbossServiceUtil.h"

#include <fmt/format.h>
#include <folly/String.h>
#include <glog/logging.h>
#include <stdexcept>
#include "fboss/agent/AgentDirectoryUtil.h"
#include "fboss/agent/if/gen-cpp2/FbossCtrl.h"
#include "fboss/cli/fboss2/session/SystemdInterface.h"
#include "fboss/cli/fboss2/utils/CmdClientUtilsCommon.h"
#include "fboss/cli/fboss2/utils/HostInfo.h"
#include "fboss/lib/CommonFileUtils.h"

namespace {
constexpr std::string_view kWedgeAgent = "wedge_agent";
constexpr std::string_view kSwAgent = "fboss_sw_agent";
constexpr std::string_view kHwAgentPrefix = "fboss_hw_agent@";
} // namespace

namespace facebook::fboss {

FbossServiceUtil::FbossServiceUtil(
    std::map<int64_t, cfg::SwitchInfo> switchInfoMap,
    bool multiSwitch)
    : systemd_(std::make_unique<SystemdInterface>()),
      switchInfoMap_(std::move(switchInfoMap)),
      multiSwitch_(multiSwitch) {}

FbossServiceUtil::FbossServiceUtil(
    std::map<int64_t, cfg::SwitchInfo> switchInfoMap,
    bool multiSwitch,
    std::unique_ptr<SystemdInterface> systemd)
    : systemd_(std::move(systemd)),
      switchInfoMap_(std::move(switchInfoMap)),
      multiSwitch_(multiSwitch) {}

std::string FbossServiceUtil::getServiceName(cli::ServiceType service) {
  switch (service) {
    case cli::ServiceType::AGENT:
      return std::string(kWedgeAgent);
  }
  throw std::runtime_error("Unknown service type");
}

bool FbossServiceUtil::isSplitMode() const {
  return multiSwitch_;
}

std::string FbossServiceUtil::getColdbootFileForService(
    const std::string& service) {
  AgentDirectoryUtil dirUtil;

  if (service == kSwAgent) {
    return dirUtil.getSwColdBootOnceFile();
  } else if (service.find(kHwAgentPrefix) == 0) {
    std::string indexStr = service.substr(kHwAgentPrefix.size());
    int switchIndex = folly::to<int>(indexStr);
    return dirUtil.getHwColdBootOnceFile(switchIndex);
  } else if (service == kWedgeAgent) {
    return dirUtil.getColdBootOnceFile();
  } else {
    throw std::runtime_error(
        fmt::format("Unknown service type for coldboot: {}", service));
  }
}

void FbossServiceUtil::createColdbootMarkerFile(
    const std::string& coldbootFile) {
  createDir(parentDirectoryTree(coldbootFile));
  touchFile(coldbootFile);
}

void FbossServiceUtil::performRestartAndWait(const std::string& service) {
  systemd_->restartService(service);
  systemd_->waitForServiceActive(service);
}

void FbossServiceUtil::performColdboot(
    const std::vector<std::string>& services) {
  for (const auto& service : services) {
    LOG(INFO) << "Performing coldboot for service: " << service;
    createColdbootMarkerFile(getColdbootFileForService(service));
    performRestartAndWait(service);
    LOG(INFO) << "Coldboot completed for service: " << service;
  }
}

void FbossServiceUtil::performWarmboot(
    const std::vector<std::string>& services) {
  for (const auto& service : services) {
    LOG(INFO) << "Performing warmboot for service: " << service;
    performRestartAndWait(service);
    LOG(INFO) << "Warmboot completed for service: " << service;
  }
}

std::vector<std::string> FbossServiceUtil::getServicesToRestart(
    cli::ServiceType service) const {
  switch (service) {
    case cli::ServiceType::AGENT: {
      std::vector<std::string> services;
      if (isSplitMode()) {
        LOG(INFO)
            << "Detected split mode (multi_switch flag is set in agent config)";

        // Add hw_agent instances first (hw before sw ordering)
        for (const auto& [switchId, switchInfo] : switchInfoMap_) {
          if (switchInfo.switchIndex().has_value()) {
            services.emplace_back(
                fmt::format("{}{}", kHwAgentPrefix, *switchInfo.switchIndex()));
          }
        }
        LOG(INFO) << "Found " << services.size() << " hw_agent instances";

        // Add sw_agent last so hw_agent restarts first
        services.emplace_back(kSwAgent);
      } else {
        LOG(INFO) << "Detected monolithic mode (multi_switch flag is not set)";
        services.emplace_back(getServiceName(service));
      }
      return services;
    }
  }
  throw std::runtime_error("Unknown service type");
}

std::vector<std::string> FbossServiceUtil::reloadConfig(
    cli::ServiceType service,
    const HostInfo& hostInfo) {
  std::vector<std::string> reloadedServices;
  switch (service) {
    case cli::ServiceType::AGENT: {
      std::string serviceName =
          isSplitMode() ? std::string(kSwAgent) : getServiceName(service);

      LOG(INFO) << "Reloading config for " << serviceName;

      auto client = utils::createClient<
          apache::thrift::Client<facebook::fboss::FbossCtrl>>(hostInfo);
      client->sync_reloadConfig();

      LOG(INFO) << "Config reloaded for " << serviceName;
      reloadedServices.emplace_back(serviceName);
      break;
    }
      // TODO: Add cases for future services (e.g., BGP)
  }
  return reloadedServices;
}

std::vector<std::string> FbossServiceUtil::restartService(
    cli::ServiceType service,
    cli::ConfigActionLevel level) {
  std::string restartType = (level == cli::ConfigActionLevel::AGENT_COLDBOOT)
      ? "coldboot"
      : "warmboot";

  auto services = getServicesToRestart(service);

  LOG(INFO) << "Restarting agents (" << restartType << ")...";

  if (level == cli::ConfigActionLevel::AGENT_COLDBOOT) {
    performColdboot(services);
  } else {
    performWarmboot(services);
  }

  return services;
}

} // namespace facebook::fboss
