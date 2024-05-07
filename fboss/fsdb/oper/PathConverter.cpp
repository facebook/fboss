// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#include "fboss/fsdb/oper/PathConverter.h"

namespace facebook::fboss::fsdb {
namespace pc_detail {
// wildcards elems get wiped during conversion using NameToPathVisitor. To get
// them back, copy over the wildcard elems from the ext path. This works
// strictly because path elems are only coverted between name/id for
// struct/variant nodes and wildcards are only valid for other containers
std::vector<OperPathElem> mergeOverWildcards(
    const std::vector<OperPathElem>& orig,
    const std::vector<std::string>& tokens) {
  DCHECK_EQ(orig.size(), tokens.size());
  std::vector<OperPathElem> path;
  path.reserve(orig.size());
  for (auto i = 0; i < orig.size(); ++i) {
    if (!orig[i].raw_ref()) {
      // orig was not a raw elem, so just copy over it
      path.push_back(orig[i]);
    } else {
      // use the converted raw path
      OperPathElem el;
      el.set_raw(tokens[i]);
      path.push_back(std::move(el));
    }
  }
  return path;
}
} // namespace pc_detail
} // namespace facebook::fboss::fsdb
