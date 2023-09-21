// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/hw/sai/impl/util.h"
#include <glog/logging.h>
#include <vector>

namespace facebook::fboss {
void findMajorMinorVersions(
    const std::string& ver,
    std::string& major,
    std::string& minor,
    int majorLength) {
  major.clear();
  minor.clear();
  int curLen = 0;
  for (char c : ver) {
    if (c == '.') {
      curLen++;
    }
    if (curLen < majorLength) {
      major.push_back(c);
    } else if (c != '.') {
      minor.push_back(c);
    }
  }
  if (minor.empty()) {
    minor.push_back('0');
  }
}

uint64_t getAsicSdkVersion(const std::string& sdkVersion) {
  std::string delim = ".";

  // SDK version is in the format of X.Y.Z
  // Major
  auto start = 0U;
  auto end = sdkVersion.find(delim);
  std::vector<uint16_t> tokens = {0, 0, 0};
  for (auto index = 0; end != std::string::npos; index++) {
    tokens[index] = std::stoi((sdkVersion.substr(start, end - start)));
    start = end + delim.length();
    end = sdkVersion.find(delim, start);
  }

  // Patch number is optional and may be part of the sdk version string.
  // Ignore the patch number if it exists.
  end = sdkVersion.find("-", start);
  end = (end == std::string::npos) ? sdkVersion.length() : end;
  tokens[2] = std::stoi((sdkVersion.substr(start, end - start)));

  CHECK_EQ(tokens.size(), 3);
  // Ignore the patch number and convert to uint64
  // 0 represent major, 1 represents minor, 2 represents micro
  auto asicSdkVersion = ASIC_SDK_VERSION(tokens[0], tokens[1], tokens[2]);
  return asicSdkVersion;
}

} // namespace facebook::fboss
