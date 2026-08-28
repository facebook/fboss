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

#include "fboss/cli/fboss2/CmdHandler.h"
#include "fboss/cli/fboss2/CmdLocalOptions.h"

#include <string>
#include <string_view>
#include <vector>

namespace facebook::fboss {

inline constexpr std::string_view kConfigGenAgentCommand = "config_gen_agent";
inline const std::string kConfigGenAgentPlatform = "--platform";
inline const std::string kConfigGenAgentProfile = "--profile";
inline const std::string kConfigGenAgentOutputDirectory = "--output-dir";

struct CmdConfigGenAgentTraits : public WriteCommandTraits {
  static constexpr utils::ObjectArgTypeId ObjectArgTypeId =
      utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ID_NONE;
  using ObjectArgType = std::monostate;
  using RetType = std::string;
  std::vector<utils::LocalOption> LocalOptions = {
      {kConfigGenAgentPlatform, "Target platform", "wedge800bact"},
      {kConfigGenAgentProfile, "Configuration profile", "hw-test"},
      {kConfigGenAgentOutputDirectory,
       "Directory in which to create agent.conf"},
  };
};

/*
 * Thin fboss2 adapter for Agent config generation. The command layer owns CLI
 * option parsing and presentation only; config construction and filesystem
 * behavior live in reusable libraries that do not depend on the CLI framework.
 */
class CmdConfigGenAgent
    : public CmdHandler<CmdConfigGenAgent, CmdConfigGenAgentTraits> {
 public:
  using RetType = CmdConfigGenAgentTraits::RetType;

  RetType queryClient(const HostInfo& hostInfo);
  void printOutput(const RetType& outputPath);
};

} // namespace facebook::fboss
