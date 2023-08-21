// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#include "fboss/thriftpath_plugin/Path.h"

namespace thriftpath {

std::vector<std::string> copyAndExtendVec(
    const std::vector<std::string>& parents,
    std::string last) {
  std::vector<std::string> out(parents);
  out.emplace_back(std::move(last));
  return out;
}

} // namespace thriftpath
