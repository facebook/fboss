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

#include <memory>
#include <variant>
#include <vector>
#include "fboss/cli/fboss2/utils/CmdUtils.h"

namespace facebook::fboss {

class CmdArgsLists {
 public:
  static std::shared_ptr<CmdArgsLists> getInstance();

  const std::vector<std::string>& at(int i) const {
    return data_[i];
  }

  std::vector<std::string>& refAt(int i) {
    if (i >= MAX_DEPTH) {
      throw std::runtime_error(
          "There is a nested command deeper than max depth, need to increase the depth");
    }
    return data_[i];
  }

  template <typename UnfilteredTypes, typename Types>
  Types getTypedArgs() {
    const auto& unfiltered =
        utils::arrayToTuple<std::tuple_size_v<UnfilteredTypes>>(data_);
    return utils::filterTupleMonostates<UnfilteredTypes>(unfiltered);
  }

 private:
  // since we do a lot of work of figuring out argument types at compile time,
  // for now we need to know the the theoretical max depth of nested subcommands
  static constexpr auto MAX_DEPTH = 5;

  std::array<std::vector<std::string>, MAX_DEPTH> data_;
};

} // namespace facebook::fboss
