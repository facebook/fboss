/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include <vector>
#include <folly/dynamic.h>

#include "fboss/agent/test/TestUtils.h"
#include "fboss/agent/state/AclEntry.h"
#include "fboss/agent/state/MatchToActionEntry.h"
#include "fboss/agent/state/TrafficPolicy.h"
#include "fboss/agent/gen-cpp2/switch_config_types.h"

#include <gtest/gtest.h>

using namespace facebook::fboss;

TEST(TrafficPolicy, serialization) {
  TrafficPolicy tp;
  std::vector<std::shared_ptr<MatchToActionEntry>> mtas;
  for (int i = 0; i < 5; i++) {
    auto mta = std::make_shared<MatchToActionEntry>();
    auto acl = std::make_shared<AclEntry>(i, folly::to<std::string>(i));
    acl->setDscp(i);
    mta->setMatcher(acl);
    cfg::MatchAction action;
    action.sendToQueue = cfg::QueueMatchAction();
    action.__isset.sendToQueue = true;
    action.sendToQueue.queueId = i;
    mta->setAction(action);
    mtas.push_back(mta);
  }
  tp.resetMatchToActions(mtas);

  auto toFolly = tp.toFollyDynamic();

  ASSERT_NE(toFolly.find("matchToActions"), toFolly.items().end());
  int i = 0;
  for (const auto& mta : toFolly["matchToActions"]) {
    ASSERT_TRUE(*(mtas[i]->getMatcher()) ==
        *(AclEntry::fromFollyDynamic(mta["matcher"])));
    ASSERT_EQ(mtas[i]->getAction().sendToQueue.queueId,
        mta["action"]["queueMatchAction"]["queueId"].asInt());
    i++;
  }

  auto fromFolly = TrafficPolicy::fromFollyDynamic(toFolly);
  ASSERT_TRUE(tp == *fromFolly);
}
