// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/Utils.h"
#include "fboss/agent/state/LabelForwardingInformationBase.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/test/LabelForwardingUtils.h"
#include "fboss/agent/test/TestUtils.h"

#include <gtest/gtest.h>

using namespace ::testing;
using namespace facebook::fboss;

TEST(LabelFIBTests, addLabelForwardingEntry) {
  auto lFib = std::make_shared<LabelForwardingInformationBase>();
  auto entry = lFib->getLabelForwardingEntryIf(5001);
  ASSERT_EQ(nullptr, lFib->getLabelForwardingEntryIf(5001));

  entry = std::make_shared<LabelForwardingEntry>(
      5001,
      StdClientIds2ClientID(StdClientIds::OPENR),
      util::getSwapLabelNextHopEntry(AdminDistance::DIRECTLY_CONNECTED));

  lFib->addNode(entry);
  EXPECT_NE(nullptr, lFib->getLabelForwardingEntryIf(5001));
}

TEST(LabelFIBTests, removeLabelForwardingEntry) {
  auto lFib = std::make_shared<LabelForwardingInformationBase>();
  lFib->addNode(std::make_shared<LabelForwardingEntry>(
      5001,
      StdClientIds2ClientID(StdClientIds::OPENR),
      util::getSwapLabelNextHopEntry(AdminDistance::DIRECTLY_CONNECTED)));
  EXPECT_NE(nullptr, lFib->getLabelForwardingEntryIf(5001));
  lFib->removeNodeIf(5001);
  EXPECT_EQ(nullptr, lFib->getLabelForwardingEntryIf(5001));
}

TEST(LabelFIBTests, updateLabelForwardingEntry) {
  auto lFib = std::make_shared<LabelForwardingInformationBase>();

  lFib->addNode(std::make_shared<LabelForwardingEntry>(
      5001,
      StdClientIds2ClientID(StdClientIds::OPENR),
      util::getSwapLabelNextHopEntry(AdminDistance::DIRECTLY_CONNECTED)));

  EXPECT_NE(nullptr, lFib->getLabelForwardingEntryIf(5001));

  auto entry = lFib->getLabelForwardingEntryIf(5001);

  entry->update(
      StdClientIds2ClientID(StdClientIds::STATIC_ROUTE),
      util::getPushLabelNextHopEntry(AdminDistance::DIRECTLY_CONNECTED));
  entry->delEntryForClient(StdClientIds2ClientID(StdClientIds::OPENR));

  lFib->updateNode(entry);
  auto updatedEntry = lFib->getLabelForwardingEntry(5001);
  for (const auto& nhop : updatedEntry->getLabelNextHop().getNextHopSet()) {
    ASSERT_TRUE(nhop.labelForwardingAction());
    EXPECT_EQ(
        nhop.labelForwardingAction()->type(),
        LabelForwardingAction::LabelForwardingType::PUSH);
  }
}

TEST(LabelFIBTests, toAndFromFollyDynamic) {
  auto lFib = std::make_shared<LabelForwardingInformationBase>();

  auto entry = std::make_shared<LabelForwardingEntry>(
      5001,
      StdClientIds2ClientID(StdClientIds::OPENR),
      util::getSwapLabelNextHopEntry(AdminDistance::DIRECTLY_CONNECTED));
  lFib->addNode(entry);

  entry->update(
      StdClientIds2ClientID(StdClientIds::STATIC_ROUTE),
      util::getPushLabelNextHopEntry(AdminDistance::DIRECTLY_CONNECTED));

  lFib->addNode(std::make_shared<LabelForwardingEntry>(
      5002,
      StdClientIds2ClientID(StdClientIds::OPENR),
      util::getPhpLabelNextHopEntry(AdminDistance::DIRECTLY_CONNECTED)));

  auto generated =
      LabelForwardingInformationBase::fromFollyDynamic(lFib->toFollyDynamic());

  EXPECT_EQ(
      *lFib->getLabelForwardingEntry(5001),
      *generated->getLabelForwardingEntry(5001));
  EXPECT_EQ(
      *lFib->getLabelForwardingEntry(5002),
      *generated->getLabelForwardingEntry(5002));
}

TEST(LabelFIBTests, forEachAdded) {
  using facebook::fboss::DeltaFunctions::forEachAdded;

  auto state = testStateA();
  auto labelFib = state->getLabelForwardingInformationBase();
  labelFib = labelFib->clone();
  auto entry = std::make_shared<LabelForwardingEntry>(
      5001,
      StdClientIds2ClientID(StdClientIds::OPENR),
      util::getSwapLabelNextHopEntry(AdminDistance::DIRECTLY_CONNECTED));
  labelFib->addNode(entry);
  auto newState = state->clone();
  newState->resetLabelForwardingInformationBase(labelFib);
  newState->publish();

  const auto& delta = StateDelta(state, newState);
  const auto& labelFibDelta = delta.getLabelForwardingInformationBaseDelta();

  forEachAdded(
      labelFibDelta,
      [=](const std::shared_ptr<LabelForwardingEntry>& addedEntry,
          const std::shared_ptr<LabelForwardingEntry> newEntry) {
        EXPECT_EQ(*newEntry, *addedEntry);
      },
      entry);
}

TEST(LabelFIBTests, forEachRemoved) {
  using facebook::fboss::DeltaFunctions::forEachRemoved;

  auto state = testStateA();
  auto labelFib = state->getLabelForwardingInformationBase();
  labelFib = labelFib->clone();
  auto entry = std::make_shared<LabelForwardingEntry>(
      5001,
      StdClientIds2ClientID(StdClientIds::OPENR),
      util::getSwapLabelNextHopEntry(AdminDistance::DIRECTLY_CONNECTED));
  labelFib->addNode(entry);

  auto oldState = state->clone();
  oldState->resetLabelForwardingInformationBase(labelFib);
  oldState->publish();

  labelFib = oldState->getLabelForwardingInformationBase();
  labelFib = labelFib->clone();
  labelFib->removeNode(5001);

  auto newState = oldState->clone();
  newState->resetLabelForwardingInformationBase(labelFib);
  newState->publish();

  const auto& delta = StateDelta(oldState, newState);
  const auto& labelFibDelta = delta.getLabelForwardingInformationBaseDelta();

  forEachRemoved(
      labelFibDelta,
      [=](const std::shared_ptr<LabelForwardingEntry>& removedEntry,
          const std::shared_ptr<LabelForwardingEntry> oldEntry) {
        EXPECT_EQ(*oldEntry, *removedEntry);
      },
      entry);
}

TEST(LabelFIBTests, forEachChanged) {
  using facebook::fboss::DeltaFunctions::forEachChanged;

  auto state = testStateA();
  auto labelFib = state->getLabelForwardingInformationBase();
  labelFib = labelFib->clone();
  auto oldEntry = std::make_shared<LabelForwardingEntry>(
      5001,
      StdClientIds2ClientID(StdClientIds::OPENR),
      util::getSwapLabelNextHopEntry(AdminDistance::DIRECTLY_CONNECTED));
  labelFib->addNode(oldEntry);

  auto oldState = state->clone();
  oldState->resetLabelForwardingInformationBase(labelFib);
  oldState->publish();

  labelFib = oldState->getLabelForwardingInformationBase();
  labelFib = labelFib->clone();

  auto newEntry = labelFib->getLabelForwardingEntry(5001);
  newEntry = newEntry->clone();
  newEntry->update(
      StdClientIds2ClientID(StdClientIds::BGPD),
      util::getPushLabelNextHopEntry(AdminDistance::DIRECTLY_CONNECTED));
  labelFib->updateNode(newEntry);

  auto newState = oldState->clone();
  newState->resetLabelForwardingInformationBase(labelFib);
  newState->publish();

  const auto& delta = StateDelta(oldState, newState);
  const auto& labelFibDelta = delta.getLabelForwardingInformationBaseDelta();

  forEachChanged(
      labelFibDelta,
      [=](const std::shared_ptr<LabelForwardingEntry>& oldEntry,
          const std::shared_ptr<LabelForwardingEntry>& newEntry,
          const std::shared_ptr<LabelForwardingEntry>& removedEntry,
          const std::shared_ptr<LabelForwardingEntry>& addedEntry) {
        EXPECT_EQ(*oldEntry, *removedEntry);
        EXPECT_EQ(*newEntry, *addedEntry);
      },
      oldEntry,
      newEntry);
}
