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
  try {
    return apache::thrift::util::enumValueOrThrow<FsdbClient>(
        clientIdUpper.c_str());
  } catch (const std::exception& ex) {
    return FsdbClient::UNSPECIFIED;
  }
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

} // namespace facebook::fboss::fsdb
