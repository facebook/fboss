// Copyright 2004-present Facebook. All Rights Reserved.

#include <fboss/agent/state/LabelForwardingInformationBase.h>

#include "fboss/agent/Utils.h"
#include "fboss/agent/if/gen-cpp2/common_types.h"
#include "fboss/agent/if/gen-cpp2/ctrl_types.h"
#include "fboss/agent/state/LabelForwardingInformationBase.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/test/HwTestHandle.h"
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
  auto routeNextHopEntry =
      RouteNextHopEntry::fromThrift(*entry->getEntryForClient(client));
  (*state)->getLabelForwardingInformationBase()->programLabel(
      state,
      entry->getID(),
      client,
      routeNextHopEntry.getAdminDistance(),
      routeNextHopEntry.getNextHopSet());
}

void removeEntryWithUnprogramLabel(
    std::shared_ptr<SwitchState>* state,
    Label label,
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
  for (const auto& nhop : updatedEntry->getForwardInfo().getNextHopSet()) {
    ASSERT_TRUE(nhop.labelForwardingAction());
    EXPECT_EQ(
        nhop.labelForwardingAction()->type(),
        LabelForwardingAction::LabelForwardingType::PUSH);
  }
}

TEST(LabelFIBTests, toAndFromFollyDynamic) {
  auto stateA = testStateA();
  auto handle = createTestHandle(stateA);
  auto sw = handle->getSw();
  auto updater = sw->getRouteUpdater();
  RouteUpdateWrapper::SyncFibFor syncFibs;

  MplsRoute route0;
  *route0.topLabel() = 5001;
  route0.adminDistance() = AdminDistance::DIRECTLY_CONNECTED;
  auto swap = util::getSwapLabelNextHopEntry(AdminDistance::DIRECTLY_CONNECTED);
  for (auto& nhop : swap.getNextHopSet()) {
    route0.nextHops()->push_back(nhop.toThrift());
  }
  updater.addRoute(ClientID::OPENR, route0);

  MplsRoute route1;
  *route1.topLabel() = 5002;
  route1.adminDistance() = AdminDistance::DIRECTLY_CONNECTED;
  auto php = util::getPhpLabelNextHopEntry(AdminDistance::DIRECTLY_CONNECTED);
  for (auto& nhop : php.getNextHopSet()) {
    route1.nextHops()->push_back(nhop.toThrift());
  }
  updater.addRoute(ClientID::OPENR, route1);

  syncFibs.insert({RouterID(0), ClientID::OPENR});
  updater.program(
      {syncFibs, RouteUpdateWrapper::SyncFibInfo::SyncFibType::MPLS_ONLY});

  auto lFib = sw->getState()->getLabelForwardingInformationBase();
  auto generated =
      LabelForwardingInformationBase::fromFollyDynamic(lFib->toFollyDynamic());

  auto ribEntry1 = lFib->getLabelForwardingEntry(5001);
  EXPECT_TRUE(
      ribEntry1->isSame(generated->getLabelForwardingEntry(5001).get()));
  auto ribEntry2 = lFib->getLabelForwardingEntry(5002);
  EXPECT_TRUE(
      ribEntry2->isSame(generated->getLabelForwardingEntry(5002).get()));

  validateNodeSerialization(*ribEntry1);
  validateNodeSerialization(*ribEntry2);
  validateNodeMapSerialization(*lFib);
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
        EXPECT_TRUE(newEntry->isSame(addedEntry.get()));
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
        EXPECT_TRUE(oldEntry->isSame(removedEntry.get()));
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
  newEntry = labelFib->cloneLabelEntry(newEntry);
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
        EXPECT_TRUE(oldEntry->isSame(removedEntry.get()));
        EXPECT_TRUE(newEntry->isSame(addedEntry.get()));
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
  entryToAdd->setResolved(
      RouteNextHopEntry::fromThrift(*entryToAdd->getBestEntry().second));

  addOrUpdateEntryWithProgramLabel(&stateA, ClientID::OPENR, entryToAdd.get());
  stateA->publish();
  auto entryAdded =
      stateA->getLabelForwardingInformationBase()->getLabelForwardingEntry(
          5001);
  EXPECT_TRUE(entryAdded->isSame(entryToAdd.get()));
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
  entryToAdd5001->setResolved(
      RouteNextHopEntry::fromThrift(*entryToAdd5001->getBestEntry().second));
  auto entryToAdd5002 = std::make_shared<LabelForwardingEntry>(
      5002,
      ClientID::OPENR,
      util::getSwapLabelNextHopEntry(AdminDistance::DIRECTLY_CONNECTED));
  entryToAdd5002->setResolved(
      RouteNextHopEntry::fromThrift((*entryToAdd5002->getBestEntry().second)));

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
  EXPECT_TRUE(entry5001->isSame(entryToAdd5001.get()));

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
  entryToAdd5001->setResolved(
      RouteNextHopEntry::fromThrift(*entryToAdd5001->getBestEntry().second));
  auto entryToAdd5002 = std::make_shared<LabelForwardingEntry>(
      5002,
      ClientID::OPENR,
      util::getSwapLabelNextHopEntry(AdminDistance::DIRECTLY_CONNECTED));
  entryToAdd5002->setResolved(
      RouteNextHopEntry::fromThrift(*entryToAdd5002->getBestEntry().second));
  auto entryToAdd5003 = std::make_shared<LabelForwardingEntry>(
      5003,
      ClientID::BGPD,
      util::getPhpLabelNextHopEntry(AdminDistance::DIRECTLY_CONNECTED));
  entryToAdd5003->setResolved(
      RouteNextHopEntry::fromThrift(*entryToAdd5003->getBestEntry().second));

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

  EXPECT_TRUE(entryToAdd5001->isSame(entryAdded5001.get()));
  EXPECT_TRUE(entryToAdd5002->isSame(entryAdded5002.get()));
  EXPECT_TRUE(entryToAdd5003->isSame(entryAdded5003.get()));

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
  EXPECT_TRUE(entryAdded5003->isSame(entry5003.get()));
}

TEST(LabelFIBTests, oneLabelManyClients) {
  auto stateA = testStateA();
  // 1) open r adds 5001 (directly connected), and bgp adds 5002 (static route)
  auto entryToAdd5001 = std::make_shared<LabelForwardingEntry>(
      5001,
      ClientID::OPENR,
      util::getPushLabelNextHopEntry(AdminDistance::DIRECTLY_CONNECTED));
  entryToAdd5001->setResolved(
      RouteNextHopEntry::fromThrift(*entryToAdd5001->getBestEntry().second));
  addOrUpdateEntryWithProgramLabel(
      &stateA, ClientID::OPENR, entryToAdd5001.get());
  auto entryToAdd5002Bgp = std::make_shared<LabelForwardingEntry>(
      5002,
      ClientID::BGPD,
      util::getPhpLabelNextHopEntry(AdminDistance::STATIC_ROUTE));
  entryToAdd5002Bgp->setResolved(
      RouteNextHopEntry::fromThrift(*entryToAdd5002Bgp->getBestEntry().second));
  addOrUpdateEntryWithProgramLabel(
      &stateA, ClientID::BGPD, entryToAdd5002Bgp.get());
  stateA->publish();

  auto entryAdded5001 =
      stateA->getLabelForwardingInformationBase()->getLabelForwardingEntry(
          5001);
  auto entryAdded5002 =
      stateA->getLabelForwardingInformationBase()->getLabelForwardingEntry(
          5002);

  EXPECT_TRUE(entryToAdd5001->isSame(entryAdded5001.get()));
  EXPECT_EQ(
      entryToAdd5002Bgp->getForwardInfo(), entryAdded5002->getForwardInfo());

  // 2) now open r adds for 5002 (directly connected)
  auto entryToAdd5002Openr = std::make_shared<LabelForwardingEntry>(
      5002,
      ClientID::OPENR,
      util::getSwapLabelNextHopEntry(AdminDistance::DIRECTLY_CONNECTED));
  entryToAdd5002Openr->setResolved(RouteNextHopEntry::fromThrift(
      *entryToAdd5002Openr->getBestEntry().second));
  SwitchState::modify(&stateA);
  addOrUpdateEntryWithProgramLabel(
      &stateA, ClientID::OPENR, entryToAdd5002Openr.get());
  stateA->publish();

  entryAdded5002 =
      stateA->getLabelForwardingInformationBase()->getLabelForwardingEntry(
          5002);

  // 3) for 5002, now directly connected next hop is preferred
  EXPECT_EQ(
      entryToAdd5002Openr->getForwardInfo(), entryAdded5002->getForwardInfo());

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
  EXPECT_EQ(entryToAdd5002Bgp->getForwardInfo(), entry5002->getForwardInfo());
}

TEST(LabelFIBTests, LabelThrifty) {
  Label label(0xacacacac);
  validateNodeSerialization<Label, true>(label);
}
