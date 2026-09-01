/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */

#include "fboss/agent/rib/RouteUpdaterUtils.h"

#include <folly/IPAddress.h>
#include <gtest/gtest.h>

namespace facebook::fboss {
namespace {

NextHop makeNextHop(
    const char* address,
    InterfaceID interface,
    NextHopRole role,
    NextHopWeight weight = UCMP_DEFAULT_WEIGHT) {
  return ResolvedNextHop(
      folly::IPAddress(address),
      interface,
      weight,
      std::nullopt,
      std::nullopt,
      std::nullopt,
      std::nullopt,
      {},
      std::nullopt,
      std::nullopt,
      std::nullopt,
      role);
}

TEST(RemoveBackupNextHopsWithMatchingPrimary, NoBackupsRemainUnchanged) {
  const RouteNextHopSet nextHops{
      makeNextHop("2001:db8::1", InterfaceID(1), NextHopRole::PRIMARY),
      makeNextHop("2001:db8::2", InterfaceID(2), NextHopRole::PRIMARY),
  };

  EXPECT_EQ(removeBackupNextHopsWithMatchingPrimary(nextHops), nextHops);
}

TEST(RemoveBackupNextHopsWithMatchingPrimary, BackupsWithoutPrimaryRemain) {
  const RouteNextHopSet nextHops{
      makeNextHop("2001:db8::1", InterfaceID(1), NextHopRole::BACKUP, 1),
      makeNextHop("2001:db8::1", InterfaceID(1), NextHopRole::BACKUP, 2),
  };

  EXPECT_EQ(removeBackupNextHopsWithMatchingPrimary(nextHops), nextHops);
}

TEST(RemoveBackupNextHopsWithMatchingPrimary, RemovesBackupBeforePrimary) {
  const auto backup =
      makeNextHop("2001:db8::1", InterfaceID(1), NextHopRole::BACKUP, 1);
  const auto primary =
      makeNextHop("2001:db8::1", InterfaceID(1), NextHopRole::PRIMARY, 2);

  EXPECT_EQ(
      removeBackupNextHopsWithMatchingPrimary({backup, primary}),
      RouteNextHopSet{primary});
}

TEST(RemoveBackupNextHopsWithMatchingPrimary, RemovesBackupAfterPrimary) {
  const auto primary =
      makeNextHop("2001:db8::1", InterfaceID(1), NextHopRole::PRIMARY, 1);
  const auto backup =
      makeNextHop("2001:db8::1", InterfaceID(1), NextHopRole::BACKUP, 2);

  EXPECT_EQ(
      removeBackupNextHopsWithMatchingPrimary({primary, backup}),
      RouteNextHopSet{primary});
}

TEST(RemoveBackupNextHopsWithMatchingPrimary, RemovesAllMatchingBackups) {
  const auto lowerWeightBackup =
      makeNextHop("2001:db8::1", InterfaceID(1), NextHopRole::BACKUP, 1);
  const auto primary =
      makeNextHop("2001:db8::1", InterfaceID(1), NextHopRole::PRIMARY, 2);
  const auto higherWeightBackup =
      makeNextHop("2001:db8::1", InterfaceID(1), NextHopRole::BACKUP, 3);

  EXPECT_EQ(
      removeBackupNextHopsWithMatchingPrimary(
          {lowerWeightBackup, primary, higherWeightBackup}),
      RouteNextHopSet{primary});
}

TEST(RemoveBackupNextHopsWithMatchingPrimary, DifferentInterfaceDoesNotMatch) {
  const auto primary =
      makeNextHop("2001:db8::1", InterfaceID(1), NextHopRole::PRIMARY);
  const auto backup =
      makeNextHop("2001:db8::1", InterfaceID(2), NextHopRole::BACKUP);
  const RouteNextHopSet nextHops{primary, backup};

  EXPECT_EQ(removeBackupNextHopsWithMatchingPrimary(nextHops), nextHops);
}

TEST(RemoveBackupNextHopsWithMatchingPrimary, DifferentAddressDoesNotMatch) {
  const auto primary =
      makeNextHop("2001:db8::1", InterfaceID(1), NextHopRole::PRIMARY);
  const auto backup =
      makeNextHop("2001:db8::2", InterfaceID(1), NextHopRole::BACKUP);
  const RouteNextHopSet nextHops{primary, backup};

  EXPECT_EQ(removeBackupNextHopsWithMatchingPrimary(nextHops), nextHops);
}

} // namespace
} // namespace facebook::fboss
