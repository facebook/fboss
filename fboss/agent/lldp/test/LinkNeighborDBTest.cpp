// Copyright 2004-present Facebook. All Rights Reserved.
#include "fboss/agent/lldp/LinkNeighborDB.h"
#include "fboss/agent/lldp/LinkNeighbor.h"

#include <gtest/gtest.h>

using namespace facebook::fboss;
using folly::MacAddress;
using std::chrono::seconds;
using std::chrono::steady_clock;

TEST(LinkNeighborDB, test) {
  LinkNeighborDB db;

  // Define a neighbor
  LinkNeighbor n1;
  MacAddress n1mac("00:11:22:33:44:55");
  n1.setProtocol(LinkProtocol::LLDP);
  n1.setLocalPort(PortID(1));
  n1.setLocalVlan(VlanID(1));
  n1.setMac(n1mac);
  n1.setChassisId("neighbor1", LldpChassisIdType::LOCALLY_ASSIGNED);
  n1.setPortId("1/1", LldpPortIdType::LOCALLY_ASSIGNED);
  n1.setSystemName("neighbor1 name");
  n1.setTTL(seconds(5));

  db.update(n1);

  auto neighbors = db.getNeighbors();
  ASSERT_EQ(1, neighbors.size());
  EXPECT_EQ(PortID(1), neighbors[0].getLocalPort());
  EXPECT_EQ(VlanID(1), neighbors[0].getLocalVlan());
  EXPECT_EQ(n1mac, neighbors[0].getMac());
  EXPECT_EQ(
      (int)LldpChassisIdType::LOCALLY_ASSIGNED,
      (int)neighbors[0].getChassisIdType());
  EXPECT_EQ("neighbor1", neighbors[0].getChassisId());
  EXPECT_EQ(
      (int)LldpPortIdType::LOCALLY_ASSIGNED, (int)neighbors[0].getPortIdType());
  EXPECT_EQ("1/1", neighbors[0].getPortId());
  EXPECT_EQ("neighbor1 name", neighbors[0].getSystemName());
  EXPECT_EQ(seconds(5), neighbors[0].getTTL());

  // Test getting by PortID
  neighbors = db.getNeighbors(PortID(2));
  EXPECT_EQ(0, neighbors.size());
  neighbors = db.getNeighbors(PortID(1));
  EXPECT_EQ(1, neighbors.size());

  // Test an update to the same neighbor
  n1.setSystemName("neighbor1 updated name");
  n1.setTTL(seconds(10));
  db.update(n1);

  neighbors = db.getNeighbors();
  ASSERT_EQ(1, neighbors.size());
  EXPECT_EQ("neighbor1 updated name", neighbors[0].getSystemName());
  EXPECT_EQ(seconds(10), neighbors[0].getTTL());

  // Test adding additional neighbors
  LinkNeighbor n2;
  MacAddress n2mac("00:11:22:33:44:02");
  n2.setProtocol(LinkProtocol::LLDP);
  n2.setLocalPort(PortID(1));
  n2.setLocalVlan(VlanID(1));
  n2.setMac(n2mac);
  n2.setChassisId("neighbor2", LldpChassisIdType::LOCALLY_ASSIGNED);
  n2.setPortId("12/3", LldpPortIdType::LOCALLY_ASSIGNED);
  n2.setSystemName("neighbor2 name");
  n2.setTTL(seconds(15));
  db.update(n2);

  LinkNeighbor n3;
  MacAddress n3mac("00:11:22:33:44:02");
  n3.setProtocol(LinkProtocol::LLDP);
  n3.setLocalPort(PortID(3));
  n3.setLocalVlan(VlanID(1));
  n3.setMac(n3mac);
  n3.setChassisId("neighbor3", LldpChassisIdType::LOCALLY_ASSIGNED);
  n3.setPortId("9/2", LldpPortIdType::LOCALLY_ASSIGNED);
  n3.setSystemName("neighbor3 name");
  n3.setTTL(seconds(20));
  db.update(n3);

  // Make the DB contains the expected number of neighbors
  neighbors = db.getNeighbors();
  ASSERT_EQ(3, neighbors.size());
  neighbors = db.getNeighbors(PortID(1));
  ASSERT_EQ(2, neighbors.size());
  neighbors = db.getNeighbors(PortID(2));
  ASSERT_EQ(0, neighbors.size());
  neighbors = db.getNeighbors(PortID(3));
  ASSERT_EQ(1, neighbors.size());

  // n1 has an expiration time 10 seconds in the future.
  // Expire it.
  db.pruneExpiredNeighbors(steady_clock::now() + seconds(11));

  neighbors = db.getNeighbors();
  ASSERT_EQ(2, neighbors.size());
  neighbors = db.getNeighbors(PortID(1));
  ASSERT_EQ(1, neighbors.size());
  EXPECT_EQ("neighbor2 name", neighbors[0].getSystemName());
  neighbors = db.getNeighbors(PortID(3));
  ASSERT_EQ(1, neighbors.size());
  EXPECT_EQ("neighbor3 name", neighbors[0].getSystemName());
}
