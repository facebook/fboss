#include <time.h>
#include <chrono>
#include <iomanip>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

#include <folly/Conv.h>
#include <folly/String.h>
#include <tabulate/table.hpp>
#include <thrift/lib/cpp/util/EnumUtils.h>
#include <thrift/lib/cpp2/async/RocketClientChannel.h>

#include "fboss/platform/helpers/InitCli.h"
#include "fboss/platform/sensor_service/if/gen-cpp2/sensor_service_clients.h"

using namespace facebook;
using namespace facebook::fboss::platform;
using namespace tabulate;

namespace {
constexpr const char* kStatusPass = "PASS";
constexpr const char* kStatusWarn = "WARN";
constexpr const char* kStatusFail = "FAIL";
constexpr const char* kPlaceholderValue = "N/A";

std::string getSensorStatus(const sensor_service::SensorData& sensor) {
  if (!sensor.value()) {
    return kStatusFail;
  }

  double value = *sensor.value();
  const auto& thresholds = sensor.thresholds();

  if (thresholds->upperCriticalVal() &&
      value > *thresholds->upperCriticalVal()) {
    return kStatusFail;
  }
  if (thresholds->lowerCriticalVal() &&
      value < *thresholds->lowerCriticalVal()) {
    return kStatusFail;
  }

  if (thresholds->maxAlarmVal() && value > *thresholds->maxAlarmVal()) {
    return kStatusWarn;
  }
  if (thresholds->minAlarmVal() && value < *thresholds->minAlarmVal()) {
    return kStatusWarn;
  }

  return kStatusPass;
}

template <typename T>
std::string formatValue(const T& val) {
  if (!val) {
    return "";
  }
  std::ostringstream oss;
  oss << std::fixed << std::setprecision(2) << *val;
  return oss.str();
}

template <typename T>
std::string formatSensorValue(const T& val) {
  if (!val) {
    return kPlaceholderValue;
  }
  std::ostringstream oss;
  oss << std::fixed << std::setprecision(2) << *val;
  return oss.str();
}

std::string formatLastUpdatedAt(const auto& timestamp) {
  if (!timestamp) {
    return "never";
  }
  auto now = std::chrono::system_clock::now();
  auto sensorTime = std::chrono::system_clock::from_time_t(*timestamp);
  auto diff = now - sensorTime;
  if (diff.count() < 0) {
    return "never";
  }
  auto seconds = std::chrono::duration_cast<std::chrono::seconds>(diff).count();
  auto timeStr = folly::prettyPrint(seconds, folly::PRETTY_TIME_HMS, false);
  return folly::rtrimWhitespace(timeStr).str() + " ago";
}

void printSensorTable(const std::vector<sensor_service::SensorData>& sensors) {
  tabulate::Table table;

  table.add_row(
      {"name",
       "value",
       "status",
       "criticalRange",
       "alarmRange",
       "lastUpdated"});

  size_t rowIndex = 1;
  for (const auto& sensor : sensors) {
    auto status = getSensorStatus(sensor);
    table.add_row(
        {*sensor.name(),
         formatSensorValue(sensor.value()),
         status,
         fmt::format(
             "[{}, {}]",
             formatValue(sensor.thresholds()->lowerCriticalVal()),
             formatValue(sensor.thresholds()->upperCriticalVal())),
         fmt::format(
             "[{}, {}]",
             formatValue(sensor.thresholds()->minAlarmVal()),
             formatValue(sensor.thresholds()->maxAlarmVal())),
         formatLastUpdatedAt(sensor.timeStamp())});
    if (status == kStatusFail) {
      table[rowIndex].format().font_color(tabulate::Color::red);
    } else if (status == kStatusWarn) {
      table[rowIndex].format().font_color(tabulate::Color::yellow);
    }
    if (rowIndex != sensors.size()) {
      table[rowIndex].format().hide_border_bottom();
    }
    if (rowIndex > 1) {
      table[rowIndex].format().hide_border_top();
    }
    rowIndex++;
  }

  table[0]
      .format()
      .font_style({tabulate::FontStyle::bold})
      .font_color(tabulate::Color::cyan);

  std::cout << table << std::endl;
}

} // namespace

// A sample sensor service plain text thrift client to be used by vendors.
// This is not for internal production use.
int main(int argc, char** argv) {
  helpers::initCli(&argc, &argv, "sensor_service_client");
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

  try {
    client->sync_getSensorValuesByNames(res, req);
  } catch (std::exception& ex) {
    std::cout << ex.what() << std::endl;
    return 1;
  }

  std::map<sensor_config::SensorType, std::vector<sensor_service::SensorData>>
      sensorsByType;
  int failureCount = 0;

  for (const auto& sensor : *res.sensorData()) {
    sensorsByType[*sensor.sensorType()].push_back(sensor);
    if (getSensorStatus(sensor) != kStatusPass) {
      failureCount++;
    }
  }

  std::cout << "\nSensor Service Output:\n";

  for (const auto& [sensorType, sensors] : sensorsByType) {
    std::cout << "\nSensor Type "
              << apache::thrift::util::enumNameSafe(sensorType) << ":\n\n";
    printSensorTable(sensors);
  }

  if (failureCount == 0) {
    std::cout << "\nAll sensors are functioning normally\n\n";
    return 0;
  } else {
    std::cout << "\n " << failureCount << " sensors failed \n\n";
    return 1;
  }
}
