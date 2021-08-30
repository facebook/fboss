// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#include "fboss/cli/fboss2/utils/clients/BmcClient.h"
#include "fboss/cli/fboss2/CmdGlobalOptions.h"

#include <folly/String.h>
#include <folly/json.h>

namespace facebook::fboss {

BmcClient::BmcClient(const HostInfo& hostInfo, int port)
    : host_(hostInfo.getOobName()), port_{port} {}

folly::dynamic BmcClient::fetchRaw(const std::string& endpoint) {
  auto result = client_.fetchUrl(buildUrl(endpoint));
  return folly::parseJson(result->getString());
}

std::string BmcClient::buildUrl(const std::string& endpoint) const {
  return fmt::format("http://{}:{}/api{}", host_, port_, endpoint);
}

} // namespace facebook::fboss
