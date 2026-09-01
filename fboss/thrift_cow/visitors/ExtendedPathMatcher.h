// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#pragma once

#include <re2/re2.h>
#include <algorithm>
#include <string>
#include <vector>

#include "fboss/fsdb/if/gen-cpp2/fsdb_oper_types.h"

namespace facebook::fboss::thrift_cow {

// Returns true if a single concrete path token matches an extended path
// element. Mirrors the semantics used by ExtendedPathVisitor:
//   - any:   matches any token
//   - raw:   exact string match
//   - regex: RE2::FullMatch
inline bool matchesStrToken(
    const std::string& tok,
    const fsdb::OperPathElem& elem,
    const re2::RE2* compiledRegex) {
  if (elem.any()) {
    return true;
  } else if (auto raw = elem.raw()) {
    return *raw == tok;
  } else if (auto regex = elem.regex()) {
    if (compiledRegex) {
      return re2::RE2::FullMatch(tok, *compiledRegex);
    } else {
      re2::RE2 re(*regex);
      return re2::RE2::FullMatch(tok, re);
    }
  }
  return false;
}

inline bool matchesStrToken(
    const std::string& tok,
    const fsdb::OperPathElem& elem) {
  return matchesStrToken(tok, elem, nullptr);
}

inline bool isWildcardElem(const fsdb::OperPathElem& elem) {
  return elem.any().has_value() || elem.regex().has_value();
}

inline bool hasWildcard(const std::vector<fsdb::OperPathElem>& path) {
  return std::any_of(path.begin(), path.end(), isWildcardElem);
}

} // namespace facebook::fboss::thrift_cow
