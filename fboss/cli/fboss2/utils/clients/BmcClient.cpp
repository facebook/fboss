// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#include "common/http_client/CurlClient.h"
#include "fboss/cli/fboss2/utils/clients/BmcClient.h"
#include "fboss/cli/fboss2/CmdGlobalOptions.h"
#include "folly/dynamic.h"

#include <folly/String.h>
#include <folly/json.h>

namespace facebook::fboss {

BmcClient::BmcClient(const HostInfo& hostInfo, int port)
    : host_(hostInfo.getOobName()), port_{port} {}

folly::dynamic BmcClient::fetchRaw(const std::string& endpoint) {
  http_client::CurlClient client;
  /* seantaylor: There are several scenarios where an endpoint may not return
                 anything.  There might be a simple timeout, or platform
                 differences where endpoints don't exist.  Adding this try/catch
                 handler to return an empty JSON instead of a nasty traceback.
                 We can implement handling empty JSON in the command itself
  */
  try {
    auto result = client.fetchUrl(buildUrl(endpoint));
    return folly::parseJson(result->getString());
  }
  catch (http_client::ErrorResponseException& ex) {
    return folly::dynamic();
  }
}

std::string BmcClient::buildUrl(const std::string& endpoint) const {
  return fmt::format("http://{}:{}/api{}", host_, port_, endpoint);
}

} // namespace facebook::fboss
