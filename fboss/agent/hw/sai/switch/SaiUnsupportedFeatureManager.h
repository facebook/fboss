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

#include "fboss/agent/FbossError.h"

#include <memory>
#include <string>

namespace facebook::fboss {

class SaiUnsupportedFeatureManager {
 public:
  explicit SaiUnsupportedFeatureManager(const std::string& name)
      : featureName_(name) {}

  template <typename Node>
  void processChanged(
      const std::shared_ptr<Node>&,
      const std::shared_ptr<Node>&) {
    throwError();
  }
  template <typename Node>
  void processAdded(const std::shared_ptr<Node>&) {
    throwError();
  }
  template <typename Node>
  void processRemoved(const std::shared_ptr<Node>&) {
    throwError();
  }

 private:
  void throwError() const {
    throw FbossError(" Feature: ", featureName_, " is not supported");
  }
  const std::string featureName_;
};

} // namespace facebook::fboss
