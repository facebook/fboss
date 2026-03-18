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

#include <string>
#include <vector>
#include "fboss/cli/fboss2/CmdList.h"

namespace facebook::fboss {

// Registration information for a command
struct CommandRegistration {
  std::string commandString; // e.g., "config vlan static-mac"
  std::string help;
  std::optional<CommandHandlerFn> commandHandler;
  std::optional<ValidFilterHandlerFn> validFilterHandler;
  std::optional<ArgTypeHandlerFn> argTypeHandler;
  std::optional<LocalOptionsHandlerFn> localOptionsHandler;
};

// Singleton registry for commands
class CommandRegistry {
 public:
  static CommandRegistry& getInstance();

  // Register a command
  void registerCommand(CommandRegistration&& registration);

  // Build the command tree from registered commands
  CommandTree buildTree();

 private:
  CommandRegistry() = default;
  std::vector<CommandRegistration> registrations_;
};

// Template class for registering commands with handlers
template <typename CmdType>
class CommandDef {
 public:
  CommandDef(const std::string& commandString, const std::string& help) {
    using Traits = typename CmdType::Traits;

    CommandRegistration reg;
    reg.commandString = commandString;
    reg.help = help;
    reg.commandHandler = commandHandler<CmdType>;
    reg.argTypeHandler = argTypeHandler<Traits>;

    // Check if the command has valid filters
    if constexpr (Traits::ALLOW_FILTERING) {
      reg.validFilterHandler = validFilterHandler<CmdType>;
    }

    // Check if the command has local options (optional feature)
    // For now, we'll skip this check and add it later if needed

    CommandRegistry::getInstance().registerCommand(std::move(reg));
  }
};

// Template class for leaf commands with no children and no handler
class CommandDefNoHandler {
 public:
  CommandDefNoHandler(const std::string& commandString, const std::string& help) {
    CommandRegistration reg;
    reg.commandString = commandString;
    reg.help = help;
    // No handlers
    CommandRegistry::getInstance().registerCommand(std::move(reg));
  }
};

// Helper for simple attributes that don't need their own class.
// The parent command handles the actual execution.
// Usage:
//   namespace {
//     AttributeDef attrDesc("config interface description",
//                           "Set interface description",
//                           utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ID_MESSAGE);
//   }
struct AttributeDef {
  AttributeDef(
      const std::string& commandString,
      const std::string& help,
      utils::ObjectArgTypeId argType =
          utils::ObjectArgTypeId::OBJECT_ARG_TYPE_ID_MESSAGE) {
    CommandRegistration reg;
    reg.commandString = commandString;
    reg.help = help;
    reg.commandHandler = std::nullopt; // No handler - parent handles it
    reg.validFilterHandler = std::nullopt;
    reg.argTypeHandler = [argType]() { return argType; };
    reg.localOptionsHandler = std::nullopt;

    CommandRegistry::getInstance().registerCommand(std::move(reg));
  }
};

} // namespace facebook::fboss

