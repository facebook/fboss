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

std::vector<facebook::fboss::fsdb::OperPathElem> copyAndExtendVec(
    const std::vector<facebook::fboss::fsdb::OperPathElem>& parents,
    facebook::fboss::fsdb::OperPathElem last) {
  std::vector<facebook::fboss::fsdb::OperPathElem> out(parents);
  out.emplace_back(std::move(last));
  return out;
}

} // namespace thriftpath
