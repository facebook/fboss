/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

package "facebook.com/fboss/cli"

namespace cpp2 facebook.fboss.cli

// Action level required for config changes to take effect.
// Used to track the highest impact action needed when committing config
// changes.
enum ConfigActionLevel {
  HITLESS = 0, // Can be applied with reloadConfig() - default
  AGENT_WARMBOOT = 1, // Requires agent warmboot restart
  AGENT_COLDBOOT = 2, // Requires agent coldboot restart (clears ASIC state)
}

// Identifier for different services that can be configured
enum ServiceType {
  AGENT = 1,
}

// Metadata stored alongside the session configuration file.
// This metadata tracks state that needs to persist across CLI invocations
// within a single config session.
struct ConfigSessionMetadata {
  // Maps each service to the required action level for pending config changes.
  // Services not in this map default to HITLESS.
  1: map<ServiceType, ConfigActionLevel> action;
}
