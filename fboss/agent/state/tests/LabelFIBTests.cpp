// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/Utils.h"
#include "fboss/agent/state/LabelForwardingInformationBase.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/test/LabelForwardingUtils.h"
#include "fboss/agent/test/TestUtils.h"

#include <gtest/gtest.h>

using namespace ::testing;
using namespace facebook::fboss;

namespace {
void addOrUpdateEntryWithProgramLabel(
    std::shared_ptr<SwitchState>* state,
    ClientID client,
    LabelForwardingEntry* entry) {
  SwitchState::modify(state);
  (*state)->getLabelForwardingInformationBase()->programLabel(
      state,
      entry->getID(),
      client,
      entry->getEntryForClient(client)->getAdminDistance(),
      entry->getEntryForClient(client)->getNextHopSet());
}

void removeEntryWithUnprogramLabel(
    std::shared_ptr<SwitchState>* state,
    LabelForwardingEntry::Label label,
    ClientID client) {
  SwitchState::modify(state);
  (*state)->getLabelForwardingInformationBase()->unprogramLabel(
      state, label, client);
}
} // namespace

TEST(LabelFIBTests, addLabelForwardingEntry) {
  auto lFib = std::make_shared<LabelForwardingInformationBase>();
  auto entry = lFib->getLabelForwardingEntryIf(5001);
  ASSERT_EQ(nullptr, lFib->getLabelForwardingEntryIf(5001));

  entry = std::make_shared<LabelForwardingEntry>(
      5001,
      ClientID::OPENR,
      util::getSwapLabelNextHopEntry(AdminDistance::DIRECTLY_CONNECTED));

  lFib->addNode(entry);
  EXPECT_NE(nullptr, lFib->getLabelForwardingEntryIf(5001));
}

TEST(LabelFIBTests, removeLabelForwardingEntry) {
  auto lFib = std::make_shared<LabelForwardingInformationBase>();
  lFib->addNode(std::make_shared<LabelForwardingEntry>(
      5001,
      ClientID::OPENR,
      util::getSwapLabelNextHopEntry(AdminDistance::DIRECTLY_CONNECTED)));
  EXPECT_NE(nullptr, lFib->getLabelForwardingEntryIf(5001));
  lFib->removeNodeIf(5001);
  EXPECT_EQ(nullptr, lFib->getLabelForwardingEntryIf(5001));
}

TEST(LabelFIBTests, updateLabelForwardingEntry) {
  auto lFib = std::make_shared<LabelForwardingInformationBase>();

  lFib->addNode(std::make_shared<LabelForwardingEntry>(
      5001,
      ClientID::OPENR,
      util::getSwapLabelNextHopEntry(AdminDistance::DIRECTLY_CONNECTED)));

  EXPECT_NE(nullptr, lFib->getLabelForwardingEntryIf(5001));

  auto entry = lFib->getLabelForwardingEntryIf(5001);

  entry->update(
      ClientID::STATIC_ROUTE,
      util::getPushLabelNextHopEntry(AdminDistance::DIRECTLY_CONNECTED));
  entry->delEntryForClient(ClientID::OPENR);

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
      ClientID::OPENR,
      util::getSwapLabelNextHopEntry(AdminDistance::DIRECTLY_CONNECTED));
  lFib->addNode(entry);

  entry->update(
      ClientID::STATIC_ROUTE,
      util::getPushLabelNextHopEntry(AdminDistance::DIRECTLY_CONNECTED));

  lFib->addNode(std::make_shared<LabelForwardingEntry>(
      5002,
      ClientID::OPENR,
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
      ClientID::OPENR,
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
      ClientID::OPENR,
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
      ClientID::OPENR,
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
      ClientID::BGPD,
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

TEST(LabelFIBTests, programLabel) {
  auto stateA = testStateA();
  auto entryToAdd = std::make_shared<LabelForwardingEntry>(
      5001,
      ClientID::OPENR,
      util::getPushLabelNextHopEntry(AdminDistance::DIRECTLY_CONNECTED));

  addOrUpdateEntryWithProgramLabel(&stateA, ClientID::OPENR, entryToAdd.get());
  stateA->publish();
  auto entryAdded =
      stateA->getLabelForwardingInformationBase()->getLabelForwardingEntry(
          5001);
  EXPECT_EQ(*entryAdded, *entryToAdd);
}

TEST(LabelFIBTests, updateLabel) {
  auto stateA = testStateA();
  auto entryToAdd = std::make_shared<LabelForwardingEntry>(
      5001,
      ClientID::OPENR,
      util::getPushLabelNextHopEntry(AdminDistance::DIRECTLY_CONNECTED));

  addOrUpdateEntryWithProgramLabel(&stateA, ClientID::OPENR, entryToAdd.get());
  stateA->publish();

  auto entryToUpdate = std::make_shared<LabelForwardingEntry>(
      5001,
      ClientID::OPENR,
      util::getSwapLabelNextHopEntry(AdminDistance::DIRECTLY_CONNECTED));

  /* updating entry for 5001 for OpenR */
  addOrUpdateEntryWithProgramLabel(
      &stateA, ClientID::OPENR, entryToUpdate.get());
  auto entryUpdated =
      stateA->getLabelForwardingInformationBase()->getLabelForwardingEntry(
          5001);

  EXPECT_EQ(
      *entryToUpdate->getEntryForClient(ClientID::OPENR),
      *entryUpdated->getEntryForClient(ClientID::OPENR));
}

TEST(LabelFIBTests, unprogramLabel) {
  auto stateA = testStateA();
  auto entryToAdd5001 = std::make_shared<LabelForwardingEntry>(
      5001,
      ClientID::OPENR,
      util::getPushLabelNextHopEntry(AdminDistance::DIRECTLY_CONNECTED));
  auto entryToAdd5002 = std::make_shared<LabelForwardingEntry>(
      5002,
      ClientID::OPENR,
      util::getSwapLabelNextHopEntry(AdminDistance::DIRECTLY_CONNECTED));

  addOrUpdateEntryWithProgramLabel(
      &stateA, ClientID::OPENR, entryToAdd5001.get());
  addOrUpdateEntryWithProgramLabel(
      &stateA, ClientID::OPENR, entryToAdd5002.get());
  stateA->publish();

  removeEntryWithUnprogramLabel(&stateA, 5002, ClientID::OPENR);
  stateA->publish();

  auto entry5001 =
      stateA->getLabelForwardingInformationBase()->getLabelForwardingEntry(
          5001);
  EXPECT_EQ(*entry5001, *entryToAdd5001);

  auto entry5002 =
      stateA->getLabelForwardingInformationBase()->getLabelForwardingEntryIf(
          5002);
  EXPECT_EQ(nullptr, entry5002);
}

TEST(LabelFIBTests, purgeEntriesForClient) {
  auto stateA = testStateA();
  auto entryToAdd5001 = std::make_shared<LabelForwardingEntry>(
      5001,
      ClientID::OPENR,
      util::getPushLabelNextHopEntry(AdminDistance::DIRECTLY_CONNECTED));
  auto entryToAdd5002 = std::make_shared<LabelForwardingEntry>(
      5002,
      ClientID::OPENR,
      util::getSwapLabelNextHopEntry(AdminDistance::DIRECTLY_CONNECTED));
  auto entryToAdd5003 = std::make_shared<LabelForwardingEntry>(
      5003,
      ClientID::BGPD,
      util::getPhpLabelNextHopEntry(AdminDistance::DIRECTLY_CONNECTED));

  addOrUpdateEntryWithProgramLabel(
      &stateA, ClientID::OPENR, entryToAdd5001.get());
  addOrUpdateEntryWithProgramLabel(
      &stateA, ClientID::OPENR, entryToAdd5002.get());
  addOrUpdateEntryWithProgramLabel(
      &stateA, ClientID::BGPD, entryToAdd5003.get());

  stateA->publish();

  auto entryAdded5001 =
      stateA->getLabelForwardingInformationBase()->getLabelForwardingEntry(
          5001);
  auto entryAdded5002 =
      stateA->getLabelForwardingInformationBase()->getLabelForwardingEntry(
          5002);
  auto entryAdded5003 =
      stateA->getLabelForwardingInformationBase()->getLabelForwardingEntry(
          5003);

  EXPECT_EQ(*entryToAdd5001, *entryAdded5001);
  EXPECT_EQ(*entryToAdd5002, *entryAdded5002);
  EXPECT_EQ(*entryToAdd5003, *entryAdded5003);

  SwitchState::modify(&stateA);
  stateA->getLabelForwardingInformationBase()->purgeEntriesForClient(
      &stateA, ClientID::OPENR);

  stateA->publish();

  auto entry5001 =
      stateA->getLabelForwardingInformationBase()->getLabelForwardingEntryIf(
          5001);
  ASSERT_EQ(nullptr, entry5001);

  auto entry5002 =
      stateA->getLabelForwardingInformationBase()->getLabelForwardingEntryIf(
          5002);
  ASSERT_EQ(nullptr, entry5002);

  auto entry5003 =
      stateA->getLabelForwardingInformationBase()->getLabelForwardingEntry(
          5003);
  EXPECT_EQ(*entryAdded5003, *entry5003);
}

TEST(LabelFIBTests, oneLabelManyClients) {
  auto stateA = testStateA();
  // 1) open r adds 5001 (directly connected), and bgp adds 5002 (static route)
  auto entryToAdd5001 = std::make_shared<LabelForwardingEntry>(
      5001,
      ClientID::OPENR,
      util::getPushLabelNextHopEntry(AdminDistance::DIRECTLY_CONNECTED));
  addOrUpdateEntryWithProgramLabel(
      &stateA, ClientID::OPENR, entryToAdd5001.get());
  auto entryToAdd5002Bgp = std::make_shared<LabelForwardingEntry>(
      5002,
      ClientID::BGPD,
      util::getPhpLabelNextHopEntry(AdminDistance::STATIC_ROUTE));
  addOrUpdateEntryWithProgramLabel(
      &stateA, ClientID::BGPD, entryToAdd5002Bgp.get());
  stateA->publish();

  auto entryAdded5001 =
      stateA->getLabelForwardingInformationBase()->getLabelForwardingEntry(
          5001);
  auto entryAdded5002 =
      stateA->getLabelForwardingInformationBase()->getLabelForwardingEntry(
          5002);

  EXPECT_EQ(*entryToAdd5001, *entryAdded5001);
  EXPECT_EQ(
      entryToAdd5002Bgp->getLabelNextHop(), entryAdded5002->getLabelNextHop());

  // 2) now open r adds for 5002 (directly connected)
  auto entryToAdd5002Openr = std::make_shared<LabelForwardingEntry>(
      5002,
      ClientID::OPENR,
      util::getSwapLabelNextHopEntry(AdminDistance::DIRECTLY_CONNECTED));
  SwitchState::modify(&stateA);
  addOrUpdateEntryWithProgramLabel(
      &stateA, ClientID::OPENR, entryToAdd5002Openr.get());
  stateA->publish();

  entryAdded5002 =
      stateA->getLabelForwardingInformationBase()->getLabelForwardingEntry(
          5002);

  // 3) for 5002, now directly connected next hop is preferred
  EXPECT_EQ(
      entryToAdd5002Openr->getLabelNextHop(),
      entryAdded5002->getLabelNextHop());

  // 4) purge entries open-r for now
  SwitchState::modify(&stateA);
  stateA->getLabelForwardingInformationBase()->purgeEntriesForClient(
      &stateA, ClientID::OPENR);
  stateA->publish();

  // 5) check that oper-r only entry is deleted
  auto entry5001 =
      stateA->getLabelForwardingInformationBase()->getLabelForwardingEntryIf(
          5001);
  ASSERT_EQ(nullptr, entry5001);

  // 6) label 5002 still has next hop published by bgp
  auto entry5002 =
      stateA->getLabelForwardingInformationBase()->getLabelForwardingEntryIf(
          5002);
  ASSERT_NE(nullptr, entry5002);

  // 7) next hop for 5002 label is now the one informed by bgp
  EXPECT_EQ(entryToAdd5002Bgp->getLabelNextHop(), entry5002->getLabelNextHop());
}
