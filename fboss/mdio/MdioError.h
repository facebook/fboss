/*
 *  Copyright (c) 2018-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#pragma once

#include <folly/Conv.h>

#include <stdexcept>
#include <string>

namespace facebook {
namespace fboss {

class MdioError : public std::exception {
 public:
  template <typename... Args>
  explicit MdioError(const Args&... args)
      : what_(folly::to<std::string>(args...)) {}

  const char* what() const noexcept override {
    return what_.c_str();
  }

 private:
  std::string what_;
};

} // namespace fboss
} // namespace facebook
