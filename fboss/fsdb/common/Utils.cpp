// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/fsdb/common/Utils.h"
#include <folly/system/ThreadName.h>

namespace facebook::fboss::fsdb {

OperDelta createDelta(std::vector<OperDeltaUnit>&& deltaUnits) {
  OperDelta delta;
  delta.changes() = deltaUnits;
  delta.protocol() = OperProtocol::BINARY;
  return delta;
}

FsdbClient string2FsdbClient(const std::string& clientId) {
  std::string clientIdUpper = clientId;
  transform(
      clientIdUpper.begin(),
      clientIdUpper.end(),
      clientIdUpper.begin(),
      ::toupper);
  FsdbClient result = FsdbClient::UNSPECIFIED;
  if (apache::thrift::util::tryParseEnum(clientIdUpper.c_str(), &result)) {
    return result;
  }
  return FsdbClient::UNSPECIFIED;
}

std::string fsdbClient2string(const FsdbClient& clientId) {
  std::string str = apache::thrift::util::enumNameSafe(clientId);
  transform(str.begin(), str.end(), str.begin(), ::tolower);
  return str;
}

ClientId subscriberId2ClientId(const SubscriberId& subscriberId) {
  ClientId clientId;

  // switch_agent uses subscriberId format of "switch_agent:<instance>"
  // parse subscriberId string with delimiter ":" to interpret it as
  // "{clientId}:{instance}"
  auto delimiter = subscriberId.find(':');
  if (delimiter != std::string::npos) {
    clientId.client() = string2FsdbClient(subscriberId.substr(0, delimiter));
    clientId.instanceId() = subscriberId.substr(delimiter + 1);
  } else {
    clientId.client() = string2FsdbClient(subscriberId);
  }

  return clientId;
}

std::string clientIdToString(const ClientId& clientId) {
  return folly::sformat(
      "{}:{}", fsdbClient2string(*clientId.client()), *clientId.instanceId());
}

SubscriberId clientId2SubscriberId(const ClientId& clientId) {
  return SubscriberId(
      folly::sformat(
          "{}:{}",
          fsdbClient2string(*clientId.client()),
          *clientId.instanceId()));
}

} // namespace facebook::fboss::fsdb
