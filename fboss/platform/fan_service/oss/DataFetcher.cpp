// Copyright 2021- Facebook. All rights reserved.
// This file contains all the function implementation
// that are OSS specific. The counterpart should be
// implemented in Meta specific file.

#include "fboss/platform/fan_service/DataFetcher.h"

#include <folly/logging/xlog.h>

#include "fboss/platform/sensor_service/if/gen-cpp2/SensorServiceThriftAsyncClient.h"
#include "fboss/platform/sensor_service/if/gen-cpp2/sensor_service_clients.h"
#include "fboss/qsfp_service/lib/QsfpClient.h"

using namespace facebook::fboss;
using namespace facebook::fboss::platform::fan_service;

namespace facebook::fboss::platform::fan_service {

void getTransceivers(
    std::map<int32_t, TransceiverInfo>& cacheTable,
    folly::EventBase& evb) {
  const std::vector<int32_t> ports{};
  auto client = std::move(QsfpClient::createClient(&evb)).getVia(&evb);
  client->sync_getTransceiverInfo(cacheTable, ports);
}

sensor_service::SensorReadResponse getSensorValueThroughThrift(
    int sensorServiceThriftPort,
    folly::EventBase& evb) {
  folly::SocketAddress sockAddr("::1", sensorServiceThriftPort);
  auto socket =
      folly::AsyncSocket::newSocket(&evb, sockAddr, kSensorSendTimeoutMs);
  auto channel =
      apache::thrift::RocketClientChannel::newChannel(std::move(socket));
  auto client = std::make_unique<apache::thrift::Client<
      facebook::fboss::platform::sensor_service::SensorServiceThrift>>(
      std::move(channel));
  sensor_service::SensorReadResponse res;
  try {
    res = client->future_getSensorValuesByNames({}).get();
  } catch (std::exception& ex) {
    XLOG(ERR) << "Exception talking to sensor_service. " << ex.what();
  }
  return res;
}

} // namespace facebook::fboss::platform::fan_service
