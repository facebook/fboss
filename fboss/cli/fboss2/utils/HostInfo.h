// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#pragma once

#include <folly/IPAddress.h>
#include "fboss/cli/fboss2/utils/CmdUtilsCommon.h"

namespace facebook::fboss {

class HostInfo {
 public:
  explicit HostInfo(const std::string& hostName)
      : HostInfo(utils::getCanonicalNameAndIPFromHost(hostName)) {}

  HostInfo(
      const std::string& hostName,
      const std::string& oob,
      const folly::IPAddress& ip)
      : name_(hostName), oob_(oob), ip_(ip) {}

  const std::string& getName() const {
    return name_;
  }

  const std::string& getOobName() const {
    return oob_;
  }

  const folly::IPAddress& getIp() const {
    return ip_;
  }

  const std::string getIpStr() const {
    return ip_.str();
  }

  bool isLocalHost() const {
    return ip_.isLoopback();
  }

 private:
  explicit HostInfo(const std::pair<std::string, folly::IPAddress>& hostAndIp)
      : HostInfo(
            hostAndIp.first,
            utils::getOobNameFromHost(hostAndIp.first),
            hostAndIp.second) {}

  const std::string name_;
  const std::string oob_;
  const folly::IPAddress ip_;
};

} // namespace facebook::fboss
