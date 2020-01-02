/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#pragma once

#include <folly/Range.h>
#include <map>
#include <string>

namespace facebook::fboss {

class BcmConfig {
 public:
  typedef std::map<std::string, std::string> ConfigMap;

  /*
   * Load the Broadcom configuration.
   *
   */
  static ConfigMap loadFromFile(const std::string& path);
  static ConfigMap loadDefaultConfig();

 private:
  static ConfigMap parseBcmStyleConfig(folly::StringPiece data);
};

} // namespace facebook::fboss
