#include <algorithm>
#include <cctype>
#include <chrono>
#include <iomanip>
#include <iostream>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <vector>

#include <CLI/CLI.hpp>
#include <folly/String.h>
#include <folly/init/Init.h>
#include <tabulate/table.hpp>
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
       "lastUpdated",
       "sysfsPath"});

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
         formatLastUpdatedAt(sensor.timeStamp()),
         *sensor.sysfsPath()});
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
  // Initialize folly with gflags disabled
  folly::InitOptions options;
  options.useGFlags(false);
  folly::Init init(&argc, &argv, options);

  CLI::App app{"Sensor service client for querying sensor data"};
  app.set_version_flag("--version", helpers::getBuildVersion());

  std::string slotPathFilter;

  app.add_option(
         "--slotpath", slotPathFilter, "Filter sensors by slotpath substring")
      ->transform([](const std::string& input) {
        std::string upper = input;
        std::transform(upper.begin(), upper.end(), upper.begin(), ::toupper);
        return upper;
      });

  try {
    app.parse(argc, argv);
  } catch (const CLI::ParseError& e) {
    return app.exit(e);
  }

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

  std::map<std::string, std::vector<sensor_service::SensorData>>
      sensorsBySlotPath;
  int failureCount = 0;

  std::set<std::string> uniqueSlotPaths;
  for (const auto& sensor : *res.sensorData()) {
    uniqueSlotPaths.insert(*sensor.slotPath());
    // Filter by slotpath if specified
    if (!slotPathFilter.empty() &&
        sensor.slotPath()->find(slotPathFilter) == std::string::npos) {
      continue;
    }

    sensorsBySlotPath[*sensor.slotPath()].push_back(sensor);
    if (getSensorStatus(sensor) != kStatusPass) {
      failureCount++;
    }
  }

  // If no sensors match the filter, show available slotpaths
  if (sensorsBySlotPath.empty() && !slotPathFilter.empty()) {
    std::cout << "No sensors found for slotpath: " << slotPathFilter
              << std::endl;
    std::cout << "\nAvailable slotpaths:" << std::endl;
    std::cout << folly::join("\n", uniqueSlotPaths) << std::endl;
    return 1;
  }

  std::cout << "\nSensor Service Output:\n";

  for (const auto& [slotPath, sensors] : sensorsBySlotPath) {
    std::cout << "\nSlot Path: " << slotPath << ":\n\n";
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
