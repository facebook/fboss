/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/lib/TupleUtils.h"

#include <gtest/gtest.h>

#include <string>
#include <tuple>

using namespace facebook::fboss;

struct A {
  int a;
};

TEST(TupleForEach, oneElement) {
  std::tuple<int> t{42};
  int sum = 0;
  tupleForEach([&sum](const auto& e) { sum += e; }, t);
  EXPECT_EQ(sum, 42);
}

TEST(TupleForEach, multipleElements) {
  std::tuple<int, double> t{42, 4.2};
  double sum = 0;
  tupleForEach([&sum](const auto& e) { sum += e; }, t);
  EXPECT_EQ(sum, 46.2);
}

TEST(TupleForEach, duplicateTypes) {
  std::tuple<int, int> t{40, 2};
  int sum = 0;
  tupleForEach([&sum](const auto& e) { sum += e; }, t);
  EXPECT_EQ(sum, 42);
}

TEST(TupleForEach, sideEffect) {
  std::tuple<A, A> t{};
  A& a1 = std::get<0>(t);
  A& a2 = std::get<1>(t);
  a1.a = 40;
  a2.a = 2;
  int sum = 0;
  tupleForEach(
      [&sum](auto& e) {
        sum += e.a;
        e.a++;
      },
      t);
  EXPECT_EQ(sum, 42);
  EXPECT_EQ(a1.a, 41);
  EXPECT_EQ(a2.a, 3);
}

TEST(TupleMap, oneElement) {
  std::tuple<int> t{42};
  auto doubled = tupleMap([](const auto& e) { return e * 2; }, t);
  EXPECT_EQ(doubled, std::tuple{84});
}

TEST(TupleMap, multipleElements) {
  std::tuple<int, double> t{42, 4.2};
  auto doubled = tupleMap([](const auto& e) { return e * 2; }, t);
  std::tuple<int, double> expected{84, 8.4};
  EXPECT_EQ(doubled, expected);
}

TEST(TupleMap, duplicateTypes) {
  std::tuple<int, int> t{42, 2};
  auto doubled = tupleMap([](const auto& e) { return e * 2; }, t);
  std::tuple<int, int> expected{84, 4};
  EXPECT_EQ(doubled, expected);
}

TEST(TupleMap, sideEffect) {
  std::tuple<A, A> t{};
  A& a1 = std::get<0>(t);
  A& a2 = std::get<1>(t);
  a1.a = 42;
  a2.a = 2;
  auto doubled = tupleMap(
      [](auto& e) {
        e.a *= 2;
        return e.a;
      },
      t);
  std::tuple<int, int> expected{84, 4};
  EXPECT_EQ(doubled, expected);
  EXPECT_EQ(a1.a, 84);
  EXPECT_EQ(a2.a, 4);
}

TEST(TupleProjection, OneToOne) {
  std::tuple<int> t1{42};
  auto t2 = tupleProjection<std::tuple<int>, std::tuple<int>>(t1);
  EXPECT_EQ(t1, t2);
}

TEST(TupleProjection, OneToZero) {
  std::tuple<int> t1{42};
  auto t2 = tupleProjection<std::tuple<int>, std::tuple<>>(t1);
  EXPECT_EQ(t2, std::tuple<>{});
}

TEST(TupleProjection, TwoToOne) {
  std::tuple<int, double> t1{42, 4.2};
  auto t2 = tupleProjection<std::tuple<int, double>, std::tuple<double>>(t1);
  EXPECT_EQ(t2, std::tuple<double>{4.2});
}

TEST(TupleProjection, ThreeToTwo) {
  std::tuple<int, double, std::string> t1{42, 4.2, "hello"};
  auto t2 = tupleProjection<
      std::tuple<int, double, std::string>,
      std::tuple<int, std::string>>(t1);
  std::tuple<int, std::string> expected{42, "hello"};
  EXPECT_EQ(t2, expected);
}

TEST(TupleProjection, ChangeOrder) {
  std::tuple<int, double, std::string> t1{42, 4.2, "hello"};
  auto t2 = tupleProjection<
      std::tuple<int, double, std::string>,
      std::tuple<std::string, int>>(t1);
  std::tuple<std::string, int> expected{"hello", 42};
  EXPECT_EQ(t2, expected);
}

TEST(TupleIndex, Test) {
  EXPECT_EQ((TupleIndex<std::tuple<int, double, std::string>, int>::value), 0);
  EXPECT_EQ(
      (TupleIndex<std::tuple<int, double, std::string>, double>::value), 1);
  EXPECT_EQ(
      (TupleIndex<std::tuple<int, double, std::string>, std::string>::value),
      2);
}
