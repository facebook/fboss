// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include <string>

#include "fboss/agent/FbossError.h"

namespace facebook::fboss {

// Service-owned directory that SDK debug dumps are confined to. API-provided
// filenames are reduced to their basename and joined onto this fixed root so a
// network caller cannot direct the (root-privileged) SDK write to an arbitrary
// path (e.g. /etc/cron.d/x, authorized_keys).
inline constexpr char kSdkDumpDir[] = "/var/facebook/fboss/sdk_dump/";

// Validate the API-provided filename and return the safe, confined path the SDK
// dump should be written to. Rejects empty and absolute inputs, then confines
// to kSdkDumpDir using only the basename so even a crafted relative path
// (including one with '..' components) cannot escape kSdkDumpDir. Reducing to
// the basename also avoids falsely rejecting legitimate names that merely
// contain ".." as a substring (e.g. "..bar").
inline std::string sanitizeSdkDumpPath(const std::string& fileName) {
  if (fileName.empty()) {
    throw FbossError("getSdkState: fileName must not be empty");
  }
  if (fileName.front() == '/') {
    throw FbossError(
        "getSdkState: absolute fileName is not allowed: ", fileName);
  }
  auto slashPos = fileName.find_last_of('/');
  auto basename =
      slashPos == std::string::npos ? fileName : fileName.substr(slashPos + 1);
  if (basename.empty() || basename == "." || basename == "..") {
    throw FbossError("getSdkState: invalid fileName: ", fileName);
  }
  return std::string(kSdkDumpDir) + basename;
}

} // namespace facebook::fboss
