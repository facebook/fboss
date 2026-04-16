// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/platform/sensor_service/PmUnitInfoFetcher.h"

#include <fmt/format.h>
#include <folly/logging/xlog.h>

using namespace facebook::fboss::platform;
namespace facebook::fboss::platform::sensor_service {

PmUnitInfoFetcher::PmUnitInfoFetcher(
    const std::shared_ptr<PmClientFactory> pmClientFactory)
    : pmClientFactory_(pmClientFactory) {}

std::optional<platform_manager::PmUnitInfo> PmUnitInfoFetcher::fetch(
    const std::string& slotPath) const {
  try {
    platform_manager::PmUnitInfoResponse resp;
    platform_manager::PmUnitInfoRequest req;
    req.slotPath() = slotPath;
    auto pmClient = pmClientFactory_->create();
    pmClient->sync_getPmUnitInfo(resp, req);
    auto pmUnitInfo = *resp.pmUnitInfo();
    if (auto pmUnitVersion = pmUnitInfo.version()) {
      XLOG(DBG1) << fmt::format(
          "Fetched PmUnitVersion: {}.{}.{} for {} at {}",
          *pmUnitVersion->productProductionState(),
          *pmUnitVersion->productVersion(),
          *pmUnitVersion->productSubVersion(),
          *pmUnitInfo.name(),
          slotPath);
    } else {
      XLOG(DBG1) << fmt::format(
          "Fetched empty PmUnitVersion for {} at {}."
          " The unit may not have an IDPROM attached.",
          *pmUnitInfo.name(),
          slotPath);
    }
    return pmUnitInfo;
  } catch (platform_manager::PlatformManagerError& error) {
    XLOG(ERR) << fmt::format(
        "Failed to get PmUnitInfo at {}. ErrorCode: {}, Message: {}",
        slotPath,
        static_cast<int>(*error.errorCode()),
        *error.message());
  } catch (std::exception& e) {
    XLOG(ERR) << fmt::format(
        "Failed to get PmUnitInfo at {}. Reason: {}", slotPath, e.what());
  }
  return std::nullopt;
}
} // namespace facebook::fboss::platform::sensor_service
