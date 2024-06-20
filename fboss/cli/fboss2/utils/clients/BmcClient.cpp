// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#include "fboss/cli/fboss2/utils/clients/BmcClient.h"
#include "common/http_client/CurlClient.h"
#include "fboss/cli/fboss2/CmdGlobalOptions.h"
#include "folly/json/dynamic.h"

#include <folly/String.h>
#include <folly/json/json.h>

namespace facebook::fboss {

BmcClient::BmcClient(const HostInfo& hostInfo, int port)
    : host_(hostInfo.getOobName()), port_{port}, endpoints_({}) {
  endpoints_["FRUID"] = "/sys/mb/fruid";
  endpoints_["SEUTIL"] = "/sys/mb/seutil";
  endpoints_["PIMSERIAL"] = "/sys/pimserial";
  endpoints_["PIMPRESENT"] = "/sys/pim_present";
  endpoints_["SEUTIL_MP2"] = "/sys/seutil";
  endpoints_["PRESENCE"] = "/sys/presence";
  endpoints_["PIMINFO"] = "/sys/piminfo";
  endpoints_["BMC"] = "/sys/bmc";
}

folly::dynamic BmcClient::fetchRaw(const std::string& endpoint) {
  http_client::CurlClient client;
  /* seantaylor: There are several scenarios where an endpoint may not return
                 anything.  There might be a simple timeout, or platform
                 differences where endpoints don't exist.  Adding this try/catch
                 handler to return an empty JSON instead of a nasty traceback.
                 We can implement handling empty JSON in the command itself
  */
  try {
    // TODO: Once BMC devices are provisioned with a non self-signed cert
    // this should be removed.
    client.allowBadCertificate(true);
    auto result = client.fetchUrl(buildUrl(endpoint));
    return folly::parseJson(result->getString());
  } catch (http_client::ErrorResponseException&) {
    return folly::dynamic();
  }
}

std::string BmcClient::buildUrl(const std::string& endpoint) const {
  return fmt::format("https://{}:{}/api{}", host_, port_, endpoint);
}

std::map<std::string, std::string> BmcClient::get_endpoints() {
  return endpoints_;
}

} // namespace facebook::fboss
