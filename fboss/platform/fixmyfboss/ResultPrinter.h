/*
 *  Copyright (c) 2004-present, Meta Platforms, Inc. and affiliates.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#pragma once

#include <iostream>
#include <string>
#include <vector>

#include "fboss/platform/platform_checks/gen-cpp2/check_types_types.h"

namespace facebook::fboss::platform::fixmyfboss {

/**
 * Utility class for printing formatted check results to the terminal.
 * Supports colored output using ANSI escape codes.
 */
class ResultPrinter {
 public:
  explicit ResultPrinter(std::ostream& out = std::cout) : out_(out) {}

  void printProgress(const std::string& message);
  void clearLine();
  void printSummary(const std::vector<platform_checks::CheckResult>& results);
  void printDetails(const std::vector<platform_checks::CheckResult>& results);

 private:
  std::ostream& out_;

  // ANSI escape sequences
  static constexpr const char* ANSI_CLEAR_LINE = "\033[K";
  static constexpr const char* ANSI_CARRIAGE_RETURN = "\r";

  // ANSI color codes
  static constexpr const char* COLOR_RED = "\033[91m";
  static constexpr const char* COLOR_GREEN = "\033[92m";
  static constexpr const char* COLOR_ORANGE = "\033[38;5;208m";
  static constexpr const char* COLOR_YELLOW = "\033[93m";
  static constexpr const char* COLOR_RESET = "\033[0m";
  static constexpr const char* STYLE_BOLD = "\033[1m";

  // ANSI background colors
  static constexpr const char* BG_RED = "\033[41m";
  static constexpr const char* BG_GREEN = "\033[42m";
  static constexpr const char* BG_ORANGE = "\033[48;5;208m";
  static constexpr const char* BG_YELLOW = "\033[43m";

  std::string
  colorize(const std::string& text, const char* color, bool bold = false);

  std::string colorizeBackground(const std::string& text, const char* bgColor);

  std::string indent(const std::string& text, int level = 1);

  std::string getStatusColor(platform_checks::CheckStatus status);
  std::string getStatusBackgroundColor(platform_checks::CheckStatus status);
  std::string getStatusName(platform_checks::CheckStatus status);
};

} // namespace facebook::fboss::platform::fixmyfboss
