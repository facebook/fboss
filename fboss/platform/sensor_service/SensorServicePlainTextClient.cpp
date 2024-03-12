// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.
#include <time.h>
#include <chrono>

#include <folly/logging/xlog.h>
#include <thrift/lib/cpp2/async/RocketClientChannel.h>

#include "fboss/platform/helpers/Init.h"
#include "fboss/platform/sensor_service/if/gen-cpp2/sensor_service_clients.h"

using namespace facebook;
using namespace facebook::fboss::platform;

namespace {
std::string tstoStr(int64_t unixTimestamp) {
  char timeBuf[100];
  struct tm ts;
  localtime_r(&unixTimestamp, &ts);
  strftime(timeBuf, sizeof(timeBuf), "%Y-%m-%d %H:%M:%S", &ts);
  return std::string(timeBuf);
}
} // namespace

// A sample sensor service plain text thrift client to be used by vendors.
// This is not for internal production use.
int main(int argc, char** argv) {
  helpers::init(&argc, &argv);
  folly::EventBase eb;
  folly::SocketAddress sockAddr("::1", 5970);
  auto socket = folly::AsyncSocket::newSocket(&eb, sockAddr, 5000);
  auto channel =
      apache::thrift::RocketClientChannel::newChannel(std::move(socket));
  auto client = std::make_unique<apache::thrift::Client<
      facebook::fboss::platform::sensor_service::SensorServiceThrift>>(
      std::move(channel));
  auto req = std::vector<std::string>{};
  facebook::fboss::platform::sensor_service::SensorReadResponse res;
  XLOG(INFO) << "Querying ...";
  try {
    client->sync_getSensorValuesByNames(res, req);
  } catch (std::exception& ex) {
    XLOG(INFO) << ex.what();
  }
  XLOG(INFO) << fmt::format("Collected data at {}", tstoStr(*res.timeStamp()));
  for (auto v : *res.sensorData()) {
    if (v.value()) {
      XLOG(INFO) << fmt::format(
          "{} -> {}. Last logged at {}",
          *v.name(),
          *v.value(),
          v.timeStamp() ? tstoStr(*v.timeStamp()) : "N/A");
    } else {
      XLOG(ERR) << fmt::format("{} -> NOT_SET. Error Reading", *v.name());
    }
  }
  return 0;
}
