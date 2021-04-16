/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/normalization/Normalizer.h"

namespace facebook::fboss {
Normalizer::Normalizer()
    : deviceName_(FLAGS_default_device_name),
      transformHandler_(std::make_unique<normalization::TransformHandler>()),
      statsExporter_(
          std::make_unique<normalization::GlogStatsExporter>(deviceName_)),
      counterTagManager_(std::make_unique<normalization::CounterTagManager>()) {
}

} // namespace facebook::fboss
