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

#include <gpiod.h>
#include <stdexcept>
#include <string>

namespace facebook::fboss {

class GpiodLineError : public std::runtime_error {
 public:
  explicit GpiodLineError(const std::string& what) : runtime_error(what) {}
};

class GpiodLine {
 public:
  GpiodLine(struct gpiod_chip* chip, uint32_t offset, const std::string& name);
  ~GpiodLine();
  int getValue();
  void setValue(int defaultVal, int targetVal);

  // Forbidden copy constructor and assignment operator
  GpiodLine(GpiodLine const&) = delete;
  GpiodLine& operator=(GpiodLine const&) = delete;

 private:
  std::string name_{};
  struct gpiod_line* line_{nullptr};
};

} // namespace facebook::fboss
