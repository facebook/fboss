/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/cli/fboss2/CommandRegistry.h"
#include <algorithm>
#include <map>
#include <sstream>
#include <stdexcept>

namespace facebook::fboss {

CommandRegistry& CommandRegistry::getInstance() {
  static CommandRegistry instance;
  return instance;
}

void CommandRegistry::registerCommand(CommandRegistration&& registration) {
  registrations_.push_back(std::move(registration));
}

namespace {

// Split command string into parts (e.g., "config vlan static-mac" -> ["config", "vlan", "static-mac"])
std::vector<std::string> splitCommandString(const std::string& commandString) {
  std::vector<std::string> parts;
  std::istringstream iss(commandString);
  std::string part;
  while (iss >> part) {
    parts.push_back(part);
  }
  return parts;
}

// Node in the command tree being built
struct TreeNode {
  std::string name;
  std::string help;
  std::optional<CommandHandlerFn> commandHandler;
  std::optional<ValidFilterHandlerFn> validFilterHandler;
  std::optional<ArgTypeHandlerFn> argTypeHandler;
  std::optional<LocalOptionsHandlerFn> localOptionsHandler;
  std::map<std::string, TreeNode> children;
  bool hasHandler = false;
};

// Insert a command into the tree
void insertCommand(
    TreeNode& root,
    const std::vector<std::string>& parts,
    size_t index,
    const CommandRegistration& reg) {
  if (index >= parts.size()) {
    return;
  }

  const std::string& name = parts[index];
  auto& child = root.children[name];

  if (child.name.empty()) {
    child.name = name;
  }

  // If this is the last part, set the command details
  if (index == parts.size() - 1) {
    // Only update if not already set (preserve existing values)
    if (child.help.empty()) {
      child.help = reg.help;
    }
    if (!child.commandHandler.has_value() && reg.commandHandler.has_value()) {
      child.commandHandler = reg.commandHandler;
    }
    if (!child.validFilterHandler.has_value() && reg.validFilterHandler.has_value()) {
      child.validFilterHandler = reg.validFilterHandler;
    }
    if (!child.argTypeHandler.has_value() && reg.argTypeHandler.has_value()) {
      child.argTypeHandler = reg.argTypeHandler;
    }
    if (!child.localOptionsHandler.has_value() && reg.localOptionsHandler.has_value()) {
      child.localOptionsHandler = reg.localOptionsHandler;
    }
    if (reg.commandHandler.has_value()) {
      child.hasHandler = true;
    }
  } else {
    // Intermediate node - continue building the tree
    insertCommand(child, parts, index + 1, reg);
  }
}

// Convert TreeNode to Command
Command treeNodeToCommand(const TreeNode& node) {
  std::vector<Command> subcommands;
  for (const auto& [childName, childNode] : node.children) {
    subcommands.push_back(treeNodeToCommand(childNode));
  }

  // Sort subcommands
  std::sort(subcommands.begin(), subcommands.end());

  // Build the Command based on what handlers are present
  if (node.commandHandler && node.validFilterHandler && node.localOptionsHandler) {
    return Command(
        node.name,
        node.help,
        *node.commandHandler,
        *node.validFilterHandler,
        *node.argTypeHandler,
        *node.localOptionsHandler,
        subcommands);
  } else if (node.commandHandler && node.validFilterHandler) {
    return Command(
        node.name,
        node.help,
        *node.commandHandler,
        *node.validFilterHandler,
        *node.argTypeHandler,
        subcommands);
  } else if (node.commandHandler && node.localOptionsHandler) {
    return Command(
        node.name,
        node.help,
        *node.commandHandler,
        *node.argTypeHandler,
        *node.localOptionsHandler,
        subcommands);
  } else if (node.commandHandler) {
    return Command(
        node.name,
        node.help,
        *node.commandHandler,
        *node.argTypeHandler,
        subcommands);
  } else {
    // No handler - parent command with only subcommands
    return Command(node.name, node.help, subcommands);
  }
}

} // namespace

CommandTree CommandRegistry::buildTree() {
  // Build a tree structure from the flat list of registrations
  std::map<std::string, TreeNode> roots; // verb -> TreeNode

  for (const auto& reg : registrations_) {
    auto parts = splitCommandString(reg.commandString);
    if (parts.empty()) {
      throw std::runtime_error("Empty command string in registration");
    }

    // First part is the verb (e.g., "config")
    const std::string& verb = parts[0];
    auto& root = roots[verb];
    if (root.name.empty()) {
      root.name = verb;
    }

    // Insert the rest of the command
    if (parts.size() > 1) {
      insertCommand(root, parts, 1, reg);
    }
  }

  // Convert the tree to CommandTree (vector of RootCommand)
  CommandTree tree;

  for (const auto& [verb, root] : roots) {
    // Each child of the root becomes a RootCommand
    for (const auto& [objectName, objectNode] : root.children) {
      Command cmd = treeNodeToCommand(objectNode);

      // Create RootCommand based on what handlers are present
      if (cmd.commandHandler && cmd.validFilterHandler && cmd.localOptionsHandler) {
        tree.push_back(RootCommand(
            verb,
            cmd.name,
            cmd.help,
            *cmd.commandHandler,
            *cmd.validFilterHandler,
            *cmd.argTypeHandler,
            *cmd.localOptionsHandler,
            cmd.subcommands));
      } else if (cmd.commandHandler && cmd.validFilterHandler) {
        tree.push_back(RootCommand(
            verb,
            cmd.name,
            cmd.help,
            *cmd.commandHandler,
            *cmd.validFilterHandler,
            *cmd.argTypeHandler,
            cmd.subcommands));
      } else if (cmd.commandHandler && cmd.localOptionsHandler) {
        tree.push_back(RootCommand(
            verb,
            cmd.name,
            cmd.help,
            *cmd.commandHandler,
            *cmd.argTypeHandler,
            *cmd.localOptionsHandler,
            cmd.subcommands));
      } else if (cmd.commandHandler) {
        tree.push_back(RootCommand(
            verb,
            cmd.name,
            cmd.help,
            *cmd.commandHandler,
            *cmd.argTypeHandler,
            cmd.subcommands));
      } else {
        // No handler - parent command with only subcommands
        tree.push_back(RootCommand(verb, cmd.name, cmd.help, cmd.subcommands));
      }
    }
  }

  std::sort(tree.begin(), tree.end());
  return tree;
}

} // namespace facebook::fboss

