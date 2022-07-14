// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include "fboss/cli/fboss2/CmdArgsLists.h"
#include "fboss/cli/fboss2/CmdList.h"

namespace facebook::fboss {

class CmdHelp {
 private:
  ReverseHelpTree tree_;

  ReverseHelpTree revHelpTree(std::vector<CommandTree>& cmdTrees) {
    using RevHelpForObj = std::map<HelpInfo, std::vector<Command>>;

    ReverseHelpTree root;
    for (auto cmdTree : cmdTrees) {
      for (const auto& cmd : cmdTree) {
        auto& objectName = cmd.name;
        auto& verb = cmd.verb;
        auto& verbHelp = cmd.help;
        auto subcommands = cmd.subcommands;
        const auto& optional = cmd.validFilterHandler;
        ValidFilterMapType validFilterMap = {};
        if (optional.has_value()) {
          const auto& validFilterMapFn = std::move(*optional);
          validFilterMap = validFilterMapFn();
        }
        RevHelpForObj& revHelp = root[objectName];
        HelpInfo helpInfo{verb, verbHelp, validFilterMap};
        // even if there are no subcommands,
        // the entry for (verb, verbHelp) must exist!
        if (subcommands.size() == 0) {
          revHelp[helpInfo] = std::vector<Command>();
        }
        for (Command c : subcommands) {
          revHelp[helpInfo].push_back(c);
        }
      }
    }
    return root;
  }

 public:
  explicit CmdHelp(std::vector<CommandTree> cmdTrees) {
    tree_ = revHelpTree(cmdTrees);
  }

  void run() {
    auto helpArgsTuple = CmdArgsLists::getInstance()->at(0);
    if (helpArgsTuple.size() != 1) {
      std::cerr << "Help takes exactly one argument" << std::endl;
      exit(EINVAL);
    }
    auto helpArg = helpArgsTuple[0];
    printHelpInfo(helpArg);
    std::cout << "Filterable fields for command show " << helpArg << " are [ ";
    printFilterInfo(helpArg);
  }

  void printSubCommandTree(
      std::vector<Command> subCmds,
      size_t level,
      std::ostream& out) {
    for (const auto& cmd : subCmds) {
      out << std::string(level - 1, '\t') << "    |---" << cmd.name << ":\t"
          << cmd.help << std::endl;
      printSubCommandTree(cmd.subcommands, level + 1, out);
    }
  }

  void printHelpInfo(std::string helpArg, std::ostream& out = std::cout) {
    const auto& helpTreeMapForObj = tree_[helpArg];
    for (const auto& [helpInfo, subCmds] : helpTreeMapForObj) {
      out << helpInfo.verb << " " << helpArg << ":\t" << helpInfo.helpMsg
          << std::endl;
      printSubCommandTree(subCmds, 1, out);
      std::cout << std::endl;
    }
  }

  void printFilterInfoHelper(const ValidFilterMapType& validFilterMap) {
    for (const auto& [key, baseTypeVerifier] : validFilterMap) {
      std::cout << key;
      const auto& acceptedValues = baseTypeVerifier->getAcceptableValues();
      if (!acceptedValues.empty()) {
        std::cout << " (Acceptable values { ";
        for (const auto& value : acceptedValues) {
          std::cout << value << " ";
        }
        std::cout << "}), ";
      } else {
        std::cout << ", ";
      }
    }
    std::cout << "]" << std::endl;
  }

  void printFilterInfo(const std::string& helpArg) {
    const auto& helpTreeMapForObj = tree_[helpArg];
    for (const auto& [helpInfo, subCmds] : helpTreeMapForObj) {
      if (helpInfo.validFilterMap.size() != 0) {
        printFilterInfoHelper(helpInfo.validFilterMap);
      }
    }
  }
};

} // namespace facebook::fboss
