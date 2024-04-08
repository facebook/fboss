/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/hw/DiagCmdFilter.h"

#include <bits/unique_ptr.h>
#include <folly/FileUtil.h>
#include <folly/String.h>
#include <folly/logging/xlog.h>
#include <re2/re2.h>
#include <fstream>
#include <sstream>

using re2::RE2;
using std::unique_ptr;

namespace facebook::fboss::utility {

DiagCmdFilter::DiagCmdFilter(
    bool isEnabled,
    std::set<std::string>& baseFilters,
    std::string& filterCfgFile)
    : baseFilters_(baseFilters) {
  enable(isEnabled);
  setCfgFile(filterCfgFile);
}

void DiagCmdFilter::enable(bool isEnabled) {
  XLOG(DBG2) << "Diag cmd filter " << (isEnabled ? "enabled" : "disabled");
  diagCmdFilterEnable_ = isEnabled;
}

bool DiagCmdFilter::isEnabled() const {
  return diagCmdFilterEnable_;
}

void DiagCmdFilter::loadDiagCmdFilters(const std::set<std::string>& filterSet) {
  // Load the cmd base filters
  for (auto str : filterSet) {
    // Line contains string of length > 0 then save it to the filter list
    if (str.size() > 0) {
      auto filter = std::make_unique<RE2>(str.c_str());
      if (!filter->ok()) {
        continue;
      }
      diagCmdAllowList_.push_back(std::move(filter));
    }
  }
}

void DiagCmdFilter::initDiagCmdFilters() {
  // Clear filters
  diagCmdAllowList_.clear();

  // Clear any cmd filters previously loaded from a cfg file
  fileCfgFilters_.clear();

  // Load the cmd base filters
  loadDiagCmdFilters(baseFilters_);
}

void DiagCmdFilter::setCfgFile(const std::string& filterCfgFile) {
  // If the file is empty just initialize base cmd filters and return
  if (filterCfgFile.empty()) {
    XLOG(DBG2) << "Diag cmd filter cfg file " << filterCfgFile << " is empty";
    initDiagCmdFilters();
    return;
  }

  // Re-initialize the filters
  initDiagCmdFilters();

  // Load the cmd filters from cfg file
  std::string data;
  if (!folly::readFile(filterCfgFile.c_str(), data)) {
    XLOG(ERR) << "Cannot read diag cmd filter cfg file: " << filterCfgFile;
    return;
  }
  folly::splitTo<std::string>(
      '\n',
      data,
      std::inserter(fileCfgFilters_, fileCfgFilters_.begin()),
      true /*ignore empty*/);

  loadDiagCmdFilters(fileCfgFilters_);
  diagCmdFilterCfgFile_ = filterCfgFile;
  XLOG(DBG2) << "Loaded diag cmd filter cfg file " << filterCfgFile;
}

std::string DiagCmdFilter::getCfgFile() const {
  return diagCmdFilterCfgFile_;
}

bool DiagCmdFilter::diagCmdAllowed(const std::string& cmd) const {
  if (diagCmdFilterEnable_) {
    return (
        std::find_if(
            diagCmdAllowList_.begin(),
            diagCmdAllowList_.end(),
            [cmd](const std::unique_ptr<RE2>& filter) {
              return RE2::FullMatch(cmd, *filter);
            }) != diagCmdAllowList_.end());
  }
  return true;
}

fbstring DiagCmdFilter::toStr() const {
  std::ostringstream os;

  os << "Diag Cmd Filte State: "
     << std::string(isEnabled() ? "enabled" : "disabled") << std::endl;

  // Get base filters
  os << "Diag Cmd Base Filters: " << std::endl;
  auto baseFilters = getBaseFilters();
  for (auto str : baseFilters) {
    if (str.size() > 0) {
      os << str << std::endl;
    }
  }

  // Get cfg file filters
  auto filterCfgFile = getCfgFile();
  os << "Diag Cmd Filter File: " << filterCfgFile << std::endl;
  auto fileCfgFilters = getFileCfgFilters();
  for (auto str : fileCfgFilters) {
    if (str.size() > 0) {
      os << str << std::endl;
    }
  }
  return os.str();
}

} // namespace facebook::fboss::utility
