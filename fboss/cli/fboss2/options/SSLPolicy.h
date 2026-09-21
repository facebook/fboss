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
#include <set>

namespace facebook::fboss {

class SSLPolicy {
 public:
  explicit SSLPolicy(const std::string& policyStr)
      : isEncrypted_{policyStr == "encrypted"} {}

  bool isEncrypted() const {
    return isEncrypted_;
  }

  static const CLI::Validator getValidator() {
    return CLI::IsMember{std::vector(getOptions())};
  }

  static const std::string getDescription() {
    return fmt::format("SSL Policy ({})", folly::join(", ", getOptions()));
  }

 private:
  static const std::vector<std::string> getOptions() {
    return {"plaintext", "encrypted"};
  }

  bool isEncrypted_ = false;
};

} // namespace facebook::fboss
