// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/platform/platform_manager/ScubaLogger.h"

#include <chrono>

#include <folly/json/dynamic.h>
#include <folly/json/json.h>
#include <folly/logging/xlog.h>
#include "common/network/NetworkUtil.h"
#include "scribe/client/ScribeClient.h"

namespace facebook::fboss::platform::platform_manager {

ScubaLogger::ScubaLogger(const std::string& platformName)
    : platformName_(platformName) {}

void ScubaLogger::log(
    const std::string& event,
    const std::unordered_map<std::string, std::string>& normals,
    const std::unordered_map<std::string, int64_t>& ints,
    const std::string& category) {
  folly::dynamic normalsObj = folly::dynamic::object;

  // Add platform name
  normalsObj["platform"] = platformName_;

  // Add standard fields
  normalsObj["hostname"] = network::NetworkUtil::getLocalHost(true);
  normalsObj["event"] = event;

  std::unordered_map<std::string, std::string> enrichedNormals = normals;

  // Add all normal fields
  for (const auto& [key, value] : enrichedNormals) {
    normalsObj[key] = value;
  }

  folly::dynamic intsObj = folly::dynamic::object;
  intsObj["time"] = std::chrono::duration_cast<std::chrono::seconds>(
                        std::chrono::system_clock::now().time_since_epoch())
                        .count();
  for (const auto& [key, value] : ints) {
    intsObj[key] = value;
  }

  folly::dynamic scubaObj = folly::dynamic::object;
  scubaObj["normal"] = normalsObj;
  scubaObj["int"] = intsObj;

  try {
    scribe::ScribeClient::getLite()->offer(category, folly::toJson(scubaObj));
    XLOG(DBG2) << "Successfully logged to Scuba (" << category
               << "): " << folly::toPrettyJson(scubaObj);
  } catch (const std::exception& e) {
    XLOG(ERR) << "Exception: " << folly::exceptionStr(e)
              << " while logging to Scuba (" << category
              << "): " << folly::toPrettyJson(scubaObj);
  }
}

void ScubaLogger::logCrash(
    const std::exception& ex,
    const std::unordered_map<std::string, std::string>& additionalFields) {
  std::unordered_map<std::string, std::string> normals = additionalFields;
  normals["error_message"] = ex.what();
  log("unexpected_crash", normals);
}

} // namespace facebook::fboss::platform::platform_manager
