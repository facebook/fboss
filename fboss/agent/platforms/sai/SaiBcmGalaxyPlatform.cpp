/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/platforms/sai/SaiBcmGalaxyPlatform.h"

#include <folly/FileUtil.h>
#include <folly/container/Array.h>
#include <thrift/lib/cpp2/protocol/Serializer.h>

DEFINE_string(
    netwhoami,
    "/etc/netwhoami.json",
    "The path to the local JSON file");

namespace {

constexpr auto kDefaultFCName = "fc001";
constexpr auto kDefaultLCName = "lc101";

std::string getDefaultCardName(bool isFabric) {
  return isFabric ? kDefaultFCName : kDefaultLCName;
}

constexpr auto kLCName = "lc_name";
// clang-format on
} // namespace

namespace facebook::fboss {

std::string SaiBcmGalaxyPlatform::getLinecardName(bool isFabric) {
  std::string netwhoamiStr;
  if (!folly::readFile(FLAGS_netwhoami.data(), netwhoamiStr)) {
    return getDefaultCardName(isFabric);
  }

  auto netwhoamiDynamic = folly::parseJson(netwhoamiStr);
  if (netwhoamiDynamic.find(kLCName) == netwhoamiDynamic.items().end()) {
    return getDefaultCardName(isFabric);
  }
  return netwhoamiDynamic[kLCName].asString();
}
} // namespace facebook::fboss
