/*
 *  Copyright (c) Meta Platforms, Inc. and affiliates.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

package "facebook.com/fboss/configs/platforms/generic/forwarding_stack"

include "fboss/lib/if/fboss_common.thrift"

namespace cpp2 facebook.fboss.configgen

enum ServiceType {
  AGENT = 1,
}

enum ConfigProfileType {
  DEFAULT = 1,
  HW_TEST = 2,
}

struct ConfigOption {
  1: ConfigProfileType profile;
  2: optional string variant;
}

struct ConfigGenerationTarget {
  1: fboss_common.PlatformType platform;
  2: map<ServiceType, list<ConfigOption>> serviceToOptions = {};
}

struct ConfigGenerationManifest {
  1: list<ConfigGenerationTarget> targets = [];
}
