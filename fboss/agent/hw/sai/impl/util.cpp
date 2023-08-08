// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/hw/sai/impl/util.h"

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
} // namespace facebook::fboss
