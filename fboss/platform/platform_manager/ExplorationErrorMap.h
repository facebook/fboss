// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include <string>
#include <unordered_map>
#include <vector>

#include "fboss/platform/platform_manager/DataStore.h"

namespace facebook::fboss::platform::platform_manager {
struct ErrorMessages {
  ErrorMessages() = default;
  ErrorMessages(bool isExpected, const std::vector<std::string>& messages)
      : isExpected_(isExpected), messages_(messages) {}
  // Return True if these error messages are expected. False if they are not.
  bool& isExpected() {
    return isExpected_;
  }
  const bool& isExpected() const {
    return isExpected_;
  }
  // Get error messages.
  std::vector<std::string>& getMessages() {
    return messages_;
  }
  const std::vector<std::string>& getMessages() const {
    return messages_;
  }

 private:
  bool isExpected_;
  std::vector<std::string> messages_;
};

typedef std::unordered_map<std::string, ErrorMessages> DeviceToErrorsMap;

struct Iterator : DeviceToErrorsMap::iterator {};

class ExplorationErrorMap {
 public:
  explicit ExplorationErrorMap(
      const PlatformConfig& config,
      const DataStore& dataStore);
  virtual ~ExplorationErrorMap() = default;
  // Add the error message happened at the devicePath.
  void add(const std::string& devicePath, const std::string& message);
  void add(
      const std::string& slotPath,
      const std::string& deviceName,
      const std::string& errorMessage);
  size_t size();
  bool empty();
  bool hasExpectedErrors();
  size_t numExpectedErrors();
  Iterator begin();
  Iterator end();

 protected:
  virtual bool isExpectedDeviceToFail(const std::string& devicePath);

 private:
  const PlatformConfig& platformConfig_;
  const DataStore& dataStore_;
  // A map of errors occured at the devicePath.
  DeviceToErrorsMap devicePathToErrors_{};
  uint nExpectedErrs{0};
};
} // namespace facebook::fboss::platform::platform_manager
