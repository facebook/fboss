// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include "fboss/cli/fboss2/CmdHandler.h"
#include "fboss/cli/fboss2/CmdList.h"

namespace facebook::fboss {

class CmdHelp {
 private:
  ReverseHelpTree tree_;

 public:
  explicit CmdHelp(ReverseHelpTree root) : tree_(root) {}
  void run() {
    // should only have one argument!
    auto helpArgsTuple = CmdArgsLists::getInstance()->at(0);
    auto helpArg = helpArgsTuple[0];
    std::cout << "help argumnents are : " << helpArgsTuple[0] << std::endl;
    auto helpTreeMapForObj = tree_[helpArg];

    for (const auto& [verbHelpPair, subCmds] : helpTreeMapForObj) {
      std::cout << verbHelpPair.first << '\t' << verbHelpPair.second
                << std::endl;
      printSubCommandTree(subCmds, 1);
    }
  }

  void printSubCommandTree(std::vector<Command> subCmds, int level) {
    for (Command cmd : subCmds) {
      std::cout << std::string(level, '\t') << cmd.name << '\t' << cmd.help;
      printSubCommandTree(cmd.subcommands, level + 1);
    }
  }
};

} // namespace facebook::fboss
