// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include "fboss/cli/fboss2/CmdArgsLists.h"
#include "fboss/cli/fboss2/CmdList.h"

namespace facebook::fboss {

class CmdHelp {
 private:
  ReverseHelpTree tree_;

  ReverseHelpTree revHelpTree(std::vector<CommandTree>& cmdTrees) {
    using RevHelpForObj =
        std::map<std::pair<CmdVerb, CmdHelpMsg>, std::vector<Command>>;

    ReverseHelpTree root;
    for (auto cmdTree : cmdTrees) {
      for (const auto& cmd : cmdTree) {
        auto& objectName = cmd.name;
        auto& verb = cmd.verb;
        auto& verbHelp = cmd.help;
        auto subcommands = cmd.subcommands;
        RevHelpForObj& revHelp = root[objectName];
        // even if there are no subcommands, the entry for (verb, verbHelp) must
        // exist!
        if (subcommands.size() == 0) {
          revHelp[make_pair(verb, verbHelp)] = std::vector<Command>();
        }
        for (Command c : subcommands) {
          revHelp[make_pair(verb, verbHelp)].push_back(c);
        }
      }
    }
    return root;
  }

  // for debugging purposes
  void printRevHelpTree() {
    for (auto element : tree_) {
      std::cout << "object: " << element.first << std::endl;
      auto revHelp = element.second;
      for (auto el2 : revHelp) {
        std::cout << " verb: " << el2.first.first
                  << " verb help: " << el2.first.second
                  << " num subcommands:  " << el2.second.size() << std::endl;
      }
    }
  }

 public:
  explicit CmdHelp(std::vector<CommandTree> cmdTrees) {
    tree_ = revHelpTree(cmdTrees);
  }

  void run() {
    // should only have one argument!
    auto helpArgsTuple = CmdArgsLists::getInstance()->at(0);
    auto helpArg = helpArgsTuple[0];
    printHelpInfo(helpArg);
  }

  void printSubCommandTree(
      std::vector<Command> subCmds,
      size_t level,
      std::ostream& out) {
    for (const auto& cmd : subCmds) {
      out << std::string(level, '\t') << cmd.name << '\t' << cmd.help
          << std::endl;
      printSubCommandTree(cmd.subcommands, level + 1, out);
    }
  }

  void printHelpInfo(auto helpArg, std::ostream& out = std::cout) {
    const auto& helpTreeMapForObj = tree_[helpArg];
    for (const auto& [verbHelpPair, subCmds] : helpTreeMapForObj) {
      out << verbHelpPair.first << '\t' << verbHelpPair.second << std::endl;
      printSubCommandTree(subCmds, 1, out);
    }
  }
};

} // namespace facebook::fboss
