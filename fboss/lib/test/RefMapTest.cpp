/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/lib/RefMap.h"

#include <gtest/gtest.h>

using namespace facebook::fboss;

// Dummy struct for placement into a RefMap
struct A {
  explicit A(int x) : x(x) {}
  int x;
};

TEST(RefMap, incRefOrEmplaceNew) {
  UnorderedRefMap<int, A> identityMap;
  bool ins;
  std::shared_ptr<A> a;
  std::tie(a, ins) = identityMap.refOrEmplace(42, 42);
  EXPECT_EQ(ins, true);
  EXPECT_EQ(a->x, 42);
  EXPECT_EQ(a.use_count(), 1);
  EXPECT_EQ(identityMap.size(), 1);
}

TEST(RefMap, incRefOrEmplaceExisting) {
  UnorderedRefMap<int, A> identityMap;
  bool ins1;
  std::shared_ptr<A> a1;
  std::tie(a1, ins1) = identityMap.refOrEmplace(42, 42);
  EXPECT_EQ(ins1, true);
  EXPECT_EQ(a1->x, 42);
  EXPECT_EQ(a1.use_count(), 1);
  EXPECT_EQ(identityMap.size(), 1);
  // second emplace should get ref to previous value
  bool ins2;
  std::shared_ptr<A> a2;
  std::tie(a2, ins2) = identityMap.refOrEmplace(42, 420);
  EXPECT_EQ(ins2, false);
  EXPECT_EQ(a2->x, 42);
  EXPECT_EQ(a1.use_count(), 2);
  EXPECT_EQ(a2.use_count(), 2);
  EXPECT_EQ(a1, a2);
  EXPECT_EQ(identityMap.size(), 1);
}

TEST(RefMap, incRefOrEmplaceMultiple) {
  UnorderedRefMap<int, A> identityMap;
  bool ins1;
  std::shared_ptr<A> a1;
  std::tie(a1, ins1) = identityMap.refOrEmplace(42, 42);
  EXPECT_EQ(ins1, true);
  EXPECT_EQ(a1->x, 42);
  EXPECT_EQ(a1.use_count(), 1);
  EXPECT_EQ(identityMap.size(), 1);
  bool ins2;
  std::shared_ptr<A> a2;
  std::tie(a2, ins2) = identityMap.refOrEmplace(420, 420);
  EXPECT_EQ(ins2, true);
  EXPECT_EQ(a2->x, 420);
  EXPECT_EQ(a2.use_count(), 1);
  EXPECT_EQ(identityMap.size(), 2);
}

TEST(RefMap, deref) {
  UnorderedRefMap<int, A> identityMap;
  {
    bool ins1;
    std::shared_ptr<A> a1;
    std::tie(a1, ins1) = identityMap.refOrEmplace(42, 42);
    EXPECT_EQ(ins1, true);
    EXPECT_EQ(a1->x, 42);
    EXPECT_EQ(a1.use_count(), 1);
    EXPECT_EQ(identityMap.size(), 1);
    {
      bool ins2;
      std::shared_ptr<A> a2;
      std::tie(a2, ins2) = identityMap.refOrEmplace(42, 420);
      EXPECT_EQ(ins2, false);
      EXPECT_EQ(a2->x, 42);
      EXPECT_EQ(a1.use_count(), 2);
      EXPECT_EQ(a2.use_count(), 2);
      EXPECT_EQ(a1, a2);
      EXPECT_EQ(identityMap.size(), 1);
    }
    EXPECT_EQ(a1.use_count(), 1);
    EXPECT_EQ(identityMap.size(), 1);
  }
  EXPECT_EQ(identityMap.size(), 0);
}

TEST(RefMap, get) {
  UnorderedRefMap<int, A> identityMap;
  bool ins;
  std::shared_ptr<A> a1;
  std::tie(a1, ins) = identityMap.refOrEmplace(42, 42);
  EXPECT_EQ(ins, true);
  EXPECT_EQ(a1->x, 42);
  EXPECT_EQ(a1.use_count(), 1);
  EXPECT_EQ(identityMap.size(), 1);
  A* a2 = identityMap.get(42);
  EXPECT_EQ(a2->x, 42);
  EXPECT_EQ(a1.use_count(), 1);
  EXPECT_EQ(identityMap.size(), 1);
}

TEST(RefMap, getNonExistent) {
  UnorderedRefMap<int, A> identityMap;
  bool ins;
  std::shared_ptr<A> a1;
  std::tie(a1, ins) = identityMap.refOrEmplace(42, 42);
  EXPECT_EQ(ins, true);
  EXPECT_EQ(a1->x, 42);
  EXPECT_EQ(a1.use_count(), 1);
  EXPECT_EQ(identityMap.size(), 1);
  A* a2 = identityMap.get(420);
  EXPECT_EQ(a2, nullptr);
  EXPECT_EQ(a1.use_count(), 1);
  EXPECT_EQ(identityMap.size(), 1);
}
