// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#pragma once

#include <folly/dynamic.h>
#include "common/http_client/CurlClient.h"
#include "fboss/cli/fboss2/utils/HostInfo.h"

namespace facebook::fboss {

class BmcClient {
 public:
  explicit BmcClient(const HostInfo& hostInfo, int port);

  folly::dynamic fetchRaw(const std::string& endpoint);

  std::string buildUrl(const std::string& endpoint) const;

 private:
  http_client::CurlClient client_;
  const std::string host_;
  int port_;
};

} // namespace facebook::fboss
