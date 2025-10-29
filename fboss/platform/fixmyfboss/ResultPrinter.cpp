/*
 *  Copyright (c) 2004-present, Meta Platforms, Inc. and affiliates.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/platform/fixmyfboss/ResultPrinter.h"

#include <sstream>

namespace facebook::fboss::platform::fixmyfboss {

void ResultPrinter::printProgress(const std::string& message) {
  out_ << ANSI_CLEAR_LINE << message << ANSI_CARRIAGE_RETURN << std::flush;
}

void ResultPrinter::clearLine() {
  out_ << ANSI_CLEAR_LINE << ANSI_CARRIAGE_RETURN << std::flush;
}

void ResultPrinter::printSummary(
    const std::vector<platform_checks::CheckResult>& results) {
  out_ << "Ran " << results.size() << " checks\n";
  out_ << "-----\n";

  // Count results by status
  int passed = 0, problems = 0, errors = 0;
  std::vector<std::string> passedNames, problemNames, errorNames;

  for (const auto& result : results) {
    const std::string name = result.checkName().value_or("Unknown");

    switch (*result.status()) {
      case platform_checks::CheckStatus::OK:
        passed++;
        passedNames.push_back(name);
        break;
      case platform_checks::CheckStatus::PROBLEM:
        problems++;
        problemNames.push_back(name);
        break;
      case platform_checks::CheckStatus::ERROR:
        errors++;
        errorNames.push_back(name);
        break;
    }
  }

  // Print counts
  out_ << "Result summary:\n";
  if (passed > 0) {
    out_ << indent(
        colorize("Passed: " + std::to_string(passed), COLOR_GREEN, true));
    out_ << "\n";
    for (const auto& name : passedNames) {
      out_ << indent(name, 2) << "\n";
    }
  }

  if (problems > 0) {
    out_ << indent(
        colorize("Problems: " + std::to_string(problems), COLOR_RED, true));
    out_ << "\n";
    for (const auto& name : problemNames) {
      out_ << indent(name, 2) << "\n";
    }
  }

  if (errors > 0) {
    out_ << indent(
        colorize("Errors: " + std::to_string(errors), COLOR_ORANGE, true));
    out_ << "\n";
    for (const auto& name : errorNames) {
      out_ << indent(name, 2) << "\n";
    }
  }

  out_ << "-----\n";
}

void ResultPrinter::printDetails(
    const std::vector<platform_checks::CheckResult>& results) {
  for (const auto& result : results) {
    // Only print details for non-OK results
    if (*result.status() == platform_checks::CheckStatus::OK) {
      continue;
    }

    const std::string name = result.checkName().value_or("Unknown");
    std::string statusBadge = colorizeBackground(
        "[  " + getStatusName(*result.status()) + "  ]",
        getStatusBackgroundColor(*result.status()).c_str());

    out_ << colorize(statusBadge, "", true) << " " << name << "\n";

    if (result.errorMessage()) {
      out_ << indent(*result.errorMessage()) << "\n";
    }

    if (result.remediation() &&
        *result.remediation() != platform_checks::RemediationType::NONE) {
      if (result.remediationMessage()) {
        out_ << indent("Remediation: " + *result.remediationMessage()) << "\n";
      }
    }
  }
}

std::string
ResultPrinter::colorize(const std::string& text, const char* color, bool bold) {
  std::string result;
  if (bold) {
    result += STYLE_BOLD;
  }
  result += color;
  result += text;
  result += COLOR_RESET;
  return result;
}

std::string ResultPrinter::colorizeBackground(
    const std::string& text,
    const char* bgColor) {
  return std::string(bgColor) + STYLE_BOLD + text + COLOR_RESET;
}

std::string ResultPrinter::indent(const std::string& text, int level) {
  std::string indentation(size_t(level) * 2, ' ');
  std::istringstream iss(text);
  std::ostringstream oss;
  std::string line;
  bool first = true;

  while (std::getline(iss, line)) {
    if (!first) {
      oss << "\n";
    }
    oss << indentation << line;
    first = false;
  }

  return oss.str();
}

std::string ResultPrinter::getStatusColor(platform_checks::CheckStatus status) {
  switch (status) {
    case platform_checks::CheckStatus::OK:
      return COLOR_GREEN;
    case platform_checks::CheckStatus::PROBLEM:
      return COLOR_RED;
    case platform_checks::CheckStatus::ERROR:
      return COLOR_ORANGE;
  }
  return COLOR_RESET;
}

std::string ResultPrinter::getStatusBackgroundColor(
    platform_checks::CheckStatus status) {
  switch (status) {
    case platform_checks::CheckStatus::OK:
      return BG_GREEN;
    case platform_checks::CheckStatus::PROBLEM:
      return BG_RED;
    case platform_checks::CheckStatus::ERROR:
      return BG_ORANGE;
  }
  return "";
}

std::string ResultPrinter::getStatusName(platform_checks::CheckStatus status) {
  switch (status) {
    case platform_checks::CheckStatus::OK:
      return "OK";
    case platform_checks::CheckStatus::PROBLEM:
      return "PROBLEM";
    case platform_checks::CheckStatus::ERROR:
      return "ERROR";
  }
  return "UNKNOWN";
}

} // namespace facebook::fboss::platform::fixmyfboss
