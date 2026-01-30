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
  AGENT_RESTART = 1, // Requires agent restart
}

// Identifier for different agents that can be configured
enum AgentType {
  WEDGE_AGENT = 1,
}

// Metadata stored alongside the session configuration file.
// This metadata tracks state that needs to persist across CLI invocations
// within a single config session.
struct ConfigSessionMetadata {
  // Maps each agent to the required action level for pending config changes.
  // Agents not in this map default to HITLESS.
  1: map<AgentType, ConfigActionLevel> action;
  // List of CLI commands executed in this session, in chronological order.
  // Each entry is the full command string (e.g., "config interface eth1/1/1 mtu 9000").
  2: list<string> commands;
  // Git commit SHA that this session is based on. Used to detect if someone
  // else committed changes while this session was in progress.
  3: string base;
}
