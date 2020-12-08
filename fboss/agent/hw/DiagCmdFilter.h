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

#include <bits/unique_ptr.h>
#include <folly/FBString.h>
#include <re2/re2.h>
#include <set>
#include <string>
#include <vector>

namespace facebook::fboss::utility {

using folly::fbstring;
using re2::RE2;
using std::unique_ptr;

class DiagCmdFilter {
 public:
  DiagCmdFilter(
      bool isEnabled,
      std::set<std::string>& baseFilters,
      std::string& filterCfgFile);
  void enable(bool isEnabled);
  bool isEnabled() const;
  void setCfgFile(const std::string& filterCfgFile);
  std::string getCfgFile() const;
  bool diagCmdAllowed(const std::string& cmd) const;
  const std::set<std::string>& getBaseFilters() const {
    return baseFilters_;
  }
  const std::set<std::string>& getFileCfgFilters() const {
    return fileCfgFilters_;
  }
  fbstring toStr() const;

 private:
  void loadDiagCmdFilters(const std::set<std::string>& filterSet);
  void initDiagCmdFilters();
  bool diagCmdFilterEnable_{false};
  std::string diagCmdFilterCfgFile_;
  std::set<std::string> baseFilters_;
  std::set<std::string> fileCfgFilters_;
  std::vector<std::unique_ptr<RE2>> diagCmdAllowList_;
};

} // namespace facebook::fboss::utility
