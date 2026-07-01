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
constexpr std::string_view kBgpPp = "bgp_pp";
} // namespace

namespace facebook::fboss {

FbossServiceUtil::FbossServiceUtil(
    std::vector<int> switchIndexes,
    bool multiSwitch)
    : systemd_(std::make_unique<SystemdInterface>()),
      switchIndexes_(std::move(switchIndexes)),
      multiSwitch_(multiSwitch) {}

FbossServiceUtil::FbossServiceUtil(
    std::vector<int> switchIndexes,
    bool multiSwitch,
    std::unique_ptr<SystemdInterface> systemd)
    : systemd_(std::move(systemd)),
      switchIndexes_(std::move(switchIndexes)),
      multiSwitch_(multiSwitch) {}

std::string FbossServiceUtil::getServiceName(cli::ServiceType service) {
  switch (service) {
    case cli::ServiceType::AGENT:
      return std::string(kWedgeAgent);
    case cli::ServiceType::BGP_PP:
      return std::string(kBgpPp);
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
            << "Detected split mode (multi-switch enabled on running agent)";

        for (const auto& switchIndex : switchIndexes_) {
          services.emplace_back(
              fmt::format("{}{}", kHwAgentPrefix, switchIndex));
        }
        LOG(INFO) << "Found " << services.size() << " hw_agent instances";

        // Add sw_agent last so hw_agent restarts first
        services.emplace_back(kSwAgent);
      } else {
        LOG(INFO)
            << "Detected monolithic mode (multi-switch not enabled on running agent)";
        services.emplace_back(getServiceName(service));
      }
      return services;
    }
    case cli::ServiceType::BGP_PP:
      // BGP++ is a single, mode-independent service.
      return {std::string(kBgpPp)};
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
    case cli::ServiceType::BGP_PP:
      // bgp_pp has no hitless reloadConfig() RPC; config changes are applied by
      // restarting the service (BGP_PP_RESTART), so this path is never taken.
      throw std::runtime_error(
          "bgp_pp does not support config reload; it must be restarted");
  }
  return reloadedServices;
}

std::vector<std::string> FbossServiceUtil::restartService(
    cli::ServiceType service,
    cli::ConfigActionLevel level) {
  std::string restartType;
  switch (level) {
    case cli::ConfigActionLevel::AGENT_COLDBOOT:
      restartType = "coldboot";
      break;
    case cli::ConfigActionLevel::AGENT_WARMBOOT:
      restartType = "warmboot";
      break;
    case cli::ConfigActionLevel::BGP_PP_RESTART:
      restartType = "restart";
      break;
    case cli::ConfigActionLevel::HITLESS:
      // Not expected: HITLESS is applied via reloadConfig(), not restart.
      restartType = "reload";
      break;
  }

  auto services = getServicesToRestart(service);

  LOG(INFO) << "Restarting " << getServiceName(service) << " (" << restartType
            << ")...";

  // Only AGENT_COLDBOOT needs the coldboot marker file; every other level is a
  // plain restart-and-wait.
  if (level == cli::ConfigActionLevel::AGENT_COLDBOOT) {
    performColdboot(services);
  } else {
    performWarmboot(services);
  }

  return services;
}

} // namespace facebook::fboss
