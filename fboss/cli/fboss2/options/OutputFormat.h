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

#include <CLI/Validators.hpp>

#include <fmt/format.h>
#include <folly/String.h>

namespace facebook::fboss {

class OutputFormat {
 public:
  OutputFormat() : OutputFormat("tabular") {}

  explicit OutputFormat(const std::string& fmt) {
    auto fmtCopy = fmt;
    folly::toLowerAscii(fmtCopy);
    isJson_ = fmtCopy == "json";
  }

  bool isJson() const {
    return isJson_;
  }

  static const CLI::Validator getValidator() {
    return CLI::IsMember(std::vector(getOptions()), CLI::ignore_case);
  }

  static const std::string getDescription() {
    return fmt::format("Output format ({})", folly::join(", ", getOptions()));
  }

 private:
  static const std::vector<std::string> getOptions() {
    return {"tabular", "json"};
  }

  bool isJson_ = false;
};

} // namespace facebook::fboss
