// Copyright 2004-present Facebook. All Rights Reserved.

#include <folly/IPAddressV4.h>
#include <folly/IPAddressV6.h>
#include <array>
#include <set>
#include <vector>
#include "Utils.h"
#include "common/base/Random.h"
#include "common/init/Init.h"
#include "fboss/lib/RadixTree.h"

using namespace std;
using namespace folly;
using namespace facebook;
using namespace facebook::network;

DEFINE_bool(v4Inserts, false, "Do inserts into v4 trees");
DEFINE_bool(v4Deletes, false, "Perform deletes on v4 trees");
DEFINE_bool(v4Exact, false, "Perform exact match on v4 trees");
DEFINE_bool(v4Longest, false, "Perform longest match on v4 trees");
DEFINE_bool(v6Inserts, false, "Do inserts into v6 trees");
DEFINE_bool(v6Deletes, false, "Perform deletes on v6 trees");
DEFINE_bool(v6Exact, false, "Perform exact match on v6 trees");
DEFINE_bool(v6Longest, false, "Perform longest match on v6 trees");

constexpr auto kTreeCount = 1000;
constexpr auto kInsertCount = 10000;
constexpr auto kMatchCount = 5000;

vector<Prefix4> insertVec4;
vector<Prefix4> matchVec4;
vector<Prefix6> insertVec6;
vector<Prefix6> matchVec6;
vector<int> valueSet;

typedef array<RadixTree<IPAddressV4, int>, kTreeCount> V4Trees_t;
typedef array<RadixTree<IPAddressV6, int>, kTreeCount> V6Trees_t;

V4Trees_t v4Trees;
V6Trees_t v6Trees;
// V4
void setupV4Trees(uint32_t numTrees = kTreeCount) {
  auto treeCount = 0;
  for (auto ritr = v4Trees.begin();
       treeCount < numTrees && ritr != v4Trees.end();
       ++ritr, ++treeCount) {
    auto count = 0;
    for (auto pfx : insertVec4) {
      ritr->insert(pfx.ip, pfx.mask, valueSet[count++]);
    }
  }
}

void radixTreeInsert4() {
  setupV4Trees();
}

void radixTreeErase4() {
  setupV4Trees();
  for (auto& rtree : v4Trees) {
    for (auto pfx : matchVec4) {
      rtree.erase(pfx.ip, pfx.mask);
    }
  }
}

void radixTreeExactMatch4() {
  setupV4Trees(1);
  auto& rtree = v4Trees[1];
  for (auto i = 0; i < kTreeCount; ++i) {
    for (auto pfx : matchVec4) {
      auto itr = rtree.exactMatch(pfx.ip, pfx.mask);
      if (!itr.atEnd()) {
        itr->setValue(folly::Random::rand32());
      }
    }
  }
}

void radixTreeLongestMatch4() {
  setupV4Trees(1);
  auto& rtree = v4Trees[1];
  for (auto i = 0; i < kTreeCount; ++i) {
    for (auto pfx : matchVec4) {
      rtree.longestMatch(pfx.ip, pfx.mask);
      auto itr = rtree.longestMatch(pfx.ip, pfx.mask);
      if (!itr.atEnd()) {
        itr->setValue(folly::Random::rand32());
      }
    }
  }
}
// V6
void setupV6Trees(uint32_t numTrees = kTreeCount) {
  auto treeCount = 0;
  for (auto ritr = v6Trees.begin();
       treeCount < numTrees && ritr != v6Trees.end();
       ++ritr, ++treeCount) {
    auto count = 0;
    for (auto pfx : insertVec6) {
      ritr->insert(pfx.ip, pfx.mask, valueSet[count++]);
    }
  }
}

void radixTreeInsert6() {
  setupV6Trees();
}

void radixTreeErase6() {
  setupV6Trees();
  for (auto& rtree : v6Trees) {
    for (auto pfx : matchVec6) {
      rtree.erase(pfx.ip, pfx.mask);
    }
  }
}

void radixTreeExactMatch6() {
  setupV6Trees(1);
  auto& rtree = v6Trees[1];
  for (auto i = 0; i < 100000; ++i) {
    for (auto pfx : matchVec6) {
      auto itr = rtree.exactMatch(pfx.ip, pfx.mask);
      if (!itr.atEnd()) {
        itr->setValue(folly::Random::rand32());
      }
    }
  }
}

void radixTreeLongestMatch6() {
  setupV6Trees(1);
  auto& rtree = v6Trees[1];
  for (auto i = 0; i < kTreeCount; ++i) {
    for (auto pfx : matchVec6) {
      auto itr = rtree.longestMatch(pfx.ip, pfx.mask);
      if (!itr.atEnd()) {
        itr->setValue(folly::Random::rand32());
      }
    }
  }
}

void fillV4MatchVec() {
  if (matchVec4.size()) {
    return;
  }
  set<Prefix4> matchSet;
  while (matchSet.size() < kMatchCount) {
    auto index = folly::Random::rand32(kInsertCount - 1);
    matchSet.insert(insertVec4[index]);
  }
  matchVec4 = {matchSet.begin(), matchSet.end()};
}

void fillV6MatchVec() {
  if (matchVec6.size()) {
    return;
  }
  set<Prefix6> matchSet;
  while (matchSet.size() < kMatchCount) {
    auto index = folly::Random::rand32(kInsertCount - 1);
    matchSet.insert(insertVec6[index]);
  }
  matchVec6 = {matchSet.begin(), matchSet.end()};
}

int main(int argc, char* argv[]) {
  facebook::initFacebook(&argc, &argv);
  // V4 ops
  if (FLAGS_v4Inserts || FLAGS_v4Deletes || FLAGS_v4Exact || FLAGS_v4Longest) {
    set<Prefix4> inserted4;
    while (inserted4.size() < kInsertCount) {
      auto mask = folly::Random::rand32(32);
      auto ip = IPAddressV4::fromLongHBO(folly::Random::rand32());
      ip = ip.mask(mask);
      if (inserted4.insert(Prefix4(ip, mask)).second) {
        valueSet.push_back(mask);
        insertVec4.push_back(Prefix4(ip, mask));
      }
    }
    if (FLAGS_v4Inserts) {
      radixTreeInsert4();
    }
    if (FLAGS_v4Deletes) {
      fillV4MatchVec();
      radixTreeErase4();
    }
    if (FLAGS_v4Exact) {
      fillV4MatchVec();
      radixTreeExactMatch4();
    }
    if (FLAGS_v4Longest) {
      fillV4MatchVec();
      radixTreeLongestMatch4();
    }
  }

  // V6 ops
  if (FLAGS_v6Inserts || FLAGS_v6Deletes || FLAGS_v6Exact || FLAGS_v6Longest) {
    set<Prefix6> inserted6;
    while (inserted6.size() < kInsertCount) {
      auto mask = folly::Random::rand32(128);
      ByteArray16 ba;
      *(uint64_t*)(&ba[0]) = folly::Random::rand64();
      *(uint64_t*)(&ba[8]) = folly::Random::rand64();
      auto ip = IPAddressV6(ba);
      ip = ip.mask(mask);
      if (inserted6.insert(Prefix6(ip, mask)).second) {
        valueSet.push_back(mask);
        insertVec6.push_back(Prefix6(ip, mask));
      }
    }
    if (FLAGS_v6Inserts) {
      radixTreeInsert6();
    }
    if (FLAGS_v6Deletes) {
      fillV6MatchVec();
      radixTreeErase6();
    }
    if (FLAGS_v6Exact) {
      fillV6MatchVec();
      radixTreeExactMatch6();
    }
    if (FLAGS_v6Longest) {
      fillV6MatchVec();
      radixTreeLongestMatch6();
    }
  }

  return 0;
}
