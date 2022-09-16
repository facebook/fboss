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

#include <memory>

namespace facebook::fboss {

class QsfpServiceDetector {
 public:
  static std::shared_ptr<QsfpServiceDetector> getInstance();

  QsfpServiceDetector();
  ~QsfpServiceDetector() {}

  bool isQsfpServiceActive() {
    return qsfpServiceActive_;
  }

  // Forbidden copy constructor and assignment operator
  QsfpServiceDetector(QsfpServiceDetector const&) = delete;
  QsfpServiceDetector& operator=(QsfpServiceDetector const&) = delete;

 private:
  void detectQsfpService();

  bool qsfpServiceActive_{false};
};

} // namespace facebook::fboss
