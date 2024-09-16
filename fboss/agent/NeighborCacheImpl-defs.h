/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#pragma once

#include <folly/IPAddress.h>
#include <folly/MacAddress.h>
#include <folly/futures/Future.h>
#include <folly/io/async/EventBase.h>
#include <folly/logging/xlog.h>
#include <list>
#include "fboss/agent/ArpHandler.h"
#include "fboss/agent/EncapIndexAllocator.h"
#include "fboss/agent/HwAsicTable.h"
#include "fboss/agent/IPv6Handler.h"
#include "fboss/agent/NeighborCacheImpl.h"
#include "fboss/agent/SwitchIdScopeResolver.h"
#include "fboss/agent/hw/switch_asics/HwAsic.h"
#include "fboss/agent/state/ArpTable.h"
#include "fboss/agent/state/NdpTable.h"
#include "fboss/agent/state/NeighborEntry.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/state/Vlan.h"
#include "fboss/agent/types.h"

DECLARE_bool(intf_nbr_tables);

namespace facebook::fboss {

namespace ncachehelpers {

/*
 * Helper that we can run to check that the interface and vlan
 * for an entry still exist and are valid.
 */
template <typename NTable>
bool checkVlanAndIntf(
    const std::shared_ptr<SwitchState>& state,
    const typename NeighborCacheEntry<NTable>::EntryFields& fields,
    std::optional<VlanID> vlanID) {
  if (vlanID.has_value()) {
    // Make sure vlan exists
    auto* vlan = state->getVlans()->getNodeIf(vlanID.value()).get();
    if (!vlan) {
      // This VLAN no longer exists.  Just ignore the entry update.
      XLOG(DBG3) << "VLAN " << vlanID.value() << " deleted before entry "
                 << fields.ip << " --> " << fields.mac << " could be updated";
      return false;
    }
  }

  // In case the interface subnets have changed, make sure the IP address
  // is still on a locally attached subnet
  if (!Interface::isIpAttached(fields.ip, fields.interfaceID, state)) {
    XLOG(DBG3) << "interface subnets changed before entry " << fields.ip
               << " --> " << fields.mac << " could be updated";
    return false;
  }

  return true;
}

inline cfg::PortDescriptor getNeighborPortDescriptor(
    const PortDescriptor& port) {
  switch (port.type()) {
    case PortDescriptor::PortType::PHYSICAL:
      return PortDescriptor(port.phyPortID()).toThrift();
    case PortDescriptor::PortType::AGGREGATE:
      return PortDescriptor(port.aggPortID()).toThrift();
    case PortDescriptor::PortType::SYSTEM_PORT:
      return PortDescriptor(port.sysPortID()).toThrift();
  }

  throw FbossError("Unnknown port descriptor");
}

} // namespace ncachehelpers

template <typename NTable>
void NeighborCacheImpl<NTable>::programEntry(Entry* entry) {
  SwSwitch::StateUpdateFn updateFn;

  auto switchType = sw_->getSwitchInfoTable().l3SwitchType();
  switch (switchType) {
    case cfg::SwitchType::NPU:
      if (FLAGS_intf_nbr_tables) {
        updateFn = getUpdateFnToProgramEntry(entry, cfg::SwitchType::NPU);
      } else {
        updateFn = getUpdateFnToProgramEntryForVlan(entry);
      }
      break;
    case cfg::SwitchType::VOQ:
      updateFn = getUpdateFnToProgramEntry(entry, cfg::SwitchType::VOQ);
      break;
    case cfg::SwitchType::FABRIC:
    case cfg::SwitchType::PHY:
      throw FbossError(
          "Programming entry is not supported for switch type: ", switchType);
  }

  sw_->updateState(
      folly::to<std::string>("add neighbor ", entry->getFields().ip),
      std::move(updateFn));
}

template <typename NTable>
SwSwitch::StateUpdateFn
NeighborCacheImpl<NTable>::getUpdateFnToProgramEntryForVlan(Entry* entry) {
  CHECK(!entry->isPending());

  auto fields = entry->getFields();
  auto vlanID = vlanID_;
  auto updateFn =
      [this, fields, vlanID](const std::shared_ptr<SwitchState>& state) mutable
      -> std::shared_ptr<SwitchState> {
    if (!ncachehelpers::checkVlanAndIntf<NTable>(state, fields, vlanID)) {
      // Either the vlan or intf is no longer valid.
      return nullptr;
    }

    auto vlan = state->getVlans()->getNodeIf(vlanID).get();
    std::shared_ptr<SwitchState> newState{state};
    auto* table = vlan->template getNeighborTable<NTable>().get();
    auto node = table->getNodeIf(fields.ip.str());

    auto isAggregatePort = fields.port.isAggregatePort();
    auto switchId = isAggregatePort
        ? sw_->getScopeResolver()
              ->scope(sw_->getState(), fields.port)
              .switchId()
        : sw_->getScopeResolver()->scope(fields.port.phyPortID()).switchId();
    auto asic = sw_->getHwAsicTable()->getHwAsicIf(switchId);
    if (asic->isSupported(HwAsic::Feature::RESERVED_ENCAP_INDEX_RANGE)) {
      fields.encapIndex =
          EncapIndexAllocator::getNextAvailableEncapIdx(state, *asic);
    }

    if (!node) {
      table = table->modify(&vlan, &newState);
      table->addEntry(fields);
      XLOG(DBG2) << "Adding entry for " << fields.ip << " --> " << fields.mac
                 << " on interface " << fields.interfaceID << " for vlan "
                 << vlanID;
    } else {
      if (node->getMac() == fields.mac && node->getPort() == fields.port &&
          node->getIntfID() == fields.interfaceID &&
          node->getState() == fields.state && !node->isPending()) {
        // This entry was already updated while we were waiting on the lock.
        return nullptr;
      }
      table = table->modify(&vlan, &newState);
      table->updateEntry(fields);
      XLOG(DBG2) << "Converting pending entry for " << fields.ip << " --> "
                 << fields.mac << " on interface " << fields.interfaceID
                 << " port " << fields.port << " for vlan " << vlanID;
    }
    return newState;
  };

  return updateFn;
}

template <typename NTable>
SwSwitch::StateUpdateFn NeighborCacheImpl<NTable>::getUpdateFnToProgramEntry(
    Entry* entry,
    cfg::SwitchType switchType) {
  CHECK(!entry->isPending());

  auto fields = entry->getFields();
  auto updateFn = [this, fields, switchType](
                      const std::shared_ptr<SwitchState>& state) mutable
      -> std::shared_ptr<SwitchState> {
    InterfaceID interfaceID;
    std::optional<SystemPortID> systemPortID;

    if (!ncachehelpers::checkVlanAndIntf<NTable>(
            state, fields, std::nullopt /* vlanID */)) {
      // intf is no longer valid.
      return nullptr;
    }

    auto isAggregatePort = fields.port.isAggregatePort();
    auto switchId = isAggregatePort
        ? sw_->getScopeResolver()
              ->scope(sw_->getState(), fields.port)
              .switchId()
        : sw_->getScopeResolver()->scope(fields.port.phyPortID()).switchId();
    auto asic = sw_->getHwAsicTable()->getHwAsicIf(switchId);
    if (asic->isSupported(HwAsic::Feature::RESERVED_ENCAP_INDEX_RANGE)) {
      fields.encapIndex =
          EncapIndexAllocator::getNextAvailableEncapIdx(state, *asic);
    }

    if (switchType == cfg::SwitchType::VOQ) {
      CHECK(!fields.port.isSystemPort());
      interfaceID = sw_->getState()->getInterfaceIDForPort(fields.port);
      // SystemPortID is always same as the InterfaceID
      systemPortID = SystemPortID(interfaceID);
    } else {
      interfaceID = intfID_;
    }

    auto newState = state->clone();
    auto intfMap = newState->getInterfaces()->modify(&newState);
    auto intf = intfMap->getNode(interfaceID);
    auto intfNew = intf->clone();
    auto nbrTable = intf->getTable<NTable>();
    auto updatedNbrTable = nbrTable->toThrift();
    auto nbrEntry = state::NeighborEntryFields();
    nbrEntry.ipaddress() = fields.ip.str();
    nbrEntry.mac() = fields.mac.toString();
    nbrEntry.interfaceId() = static_cast<uint32_t>(interfaceID);
    nbrEntry.state() = static_cast<state::NeighborState>(fields.state);
    if (fields.encapIndex.has_value()) {
      nbrEntry.encapIndex() = fields.encapIndex.value();
    }

    if (switchType == cfg::SwitchType::VOQ) {
      // TODO: Support aggregate ports for VOQ switches
      CHECK(fields.port.isPhysicalPort());
      CHECK(systemPortID.has_value());
      nbrEntry.portId() =
          PortDescriptor(SystemPortID(systemPortID.value())).toThrift();
      nbrEntry.isLocal() = fields.isLocal;
    } else {
      nbrEntry.portId() = ncachehelpers::getNeighborPortDescriptor(fields.port);
    }

    if (fields.classID.has_value()) {
      nbrEntry.classID() = fields.classID.value();
    }

    auto node = nbrTable->getEntryIf(fields.ip);
    if (!node) {
      updatedNbrTable.insert({fields.ip.str(), nbrEntry});
      XLOG(DBG2)
          << "Adding entry for: "
          << apache::thrift::SimpleJSONSerializer::serialize<std::string>(
                 nbrEntry);
    } else {
      if (node->getMac() == fields.mac &&
          ((switchType != cfg::SwitchType::VOQ &&
            node->getPort().phyPortID() == fields.port.phyPortID()) ||
           (switchType == cfg::SwitchType::VOQ &&
            node->getPort().sysPortIDIf() == systemPortID)) &&
          node->getIntfID() == interfaceID &&
          node->getState() == fields.state && !node->isPending() &&
          node->getClassID() == fields.classID) {
        return nullptr;
      }
      updatedNbrTable.erase(fields.ip.str());
      updatedNbrTable.insert({fields.ip.str(), nbrEntry});
      XLOG(DBG2)
          << "Updating entry for: "
          << apache::thrift::SimpleJSONSerializer::serialize<std::string>(
                 nbrEntry);
    }

    intfNew->setNeighborTable<NTable>(updatedNbrTable);
    intfMap->updateNode(
        intfNew, sw_->getScopeResolver()->scope(intfNew, newState));

    return newState;
  };

  return updateFn;
}

template <typename NTable>
void NeighborCacheImpl<NTable>::programPendingEntry(
    Entry* entry,
    PortDescriptor port,
    bool force) {
  SwSwitch::StateUpdateFn updateFn;

  auto switchType = sw_->getSwitchInfoTable().l3SwitchType();
  switch (switchType) {
    case cfg::SwitchType::NPU:
      if (FLAGS_intf_nbr_tables) {
        updateFn = getUpdateFnToProgramPendingEntry(
            entry, port, force, cfg::SwitchType::NPU);
      } else {
        updateFn = getUpdateFnToProgramPendingEntryForVlan(entry, port, force);
      }
      break;
    case cfg::SwitchType::VOQ:
      updateFn = getUpdateFnToProgramPendingEntry(
          entry, port, force, cfg::SwitchType::VOQ);
      break;
    case cfg::SwitchType::FABRIC:
    case cfg::SwitchType::PHY:
      throw FbossError(
          "Programming entry is not supported for switch type: ", switchType);
  }

  sw_->updateStateNoCoalescing(
      folly::to<std::string>("add pending entry ", entry->getFields().ip),
      std::move(updateFn));
}

template <typename NTable>
SwSwitch::StateUpdateFn
NeighborCacheImpl<NTable>::getUpdateFnToProgramPendingEntryForVlan(
    Entry* entry,
    PortDescriptor /*port*/,
    bool force) {
  CHECK(entry->isPending());

  auto fields = entry->getFields();
  auto vlanID = vlanID_;
  auto updateFn =
      [fields, vlanID, force](const std::shared_ptr<SwitchState>& state)
      -> std::shared_ptr<SwitchState> {
    if (!ncachehelpers::checkVlanAndIntf<NTable>(state, fields, vlanID)) {
      // Either the vlan or intf is no longer valid.
      return nullptr;
    }

    auto vlan = state->getVlans()->getNodeIf(vlanID).get();
    std::shared_ptr<SwitchState> newState{state};
    auto* table = vlan->template getNeighborTable<NTable>().get();
    auto node = table->getNodeIf(fields.ip.str());
    table = table->modify(&vlan, &newState);

    if (node) {
      if (!force) {
        // don't replace an existing entry with a pending one unless
        // explicitly allowed
        return nullptr;
      }
      table->removeEntry(fields.ip);
    }
    table->addPendingEntry(fields.ip, fields.interfaceID);

    XLOG(DBG4) << "Adding pending entry for " << fields.ip << " on interface "
               << fields.interfaceID << " for vlan " << vlanID;
    return newState;
  };

  return updateFn;
}

template <typename NTable>
SwSwitch::StateUpdateFn
NeighborCacheImpl<NTable>::getUpdateFnToProgramPendingEntry(
    Entry* entry,
    PortDescriptor port,
    bool force,
    cfg::SwitchType switchType) {
  auto fields = entry->getFields();
  auto updateFn = [this, fields, port, force, switchType](
                      const std::shared_ptr<SwitchState>& state)
      -> std::shared_ptr<SwitchState> {
    if (port.isPhysicalPort() && port.phyPortID() == PortID(0)) {
      // If the entry is pointing to the CPU port, it is not programmed in the
      // hardware, thus no-op.
      return nullptr;
    }

    if (!ncachehelpers::checkVlanAndIntf<NTable>(
            state, fields, std::nullopt /* vlanID */)) {
      // intf is no longer valid.
      return nullptr;
    }

    InterfaceID interfaceID;
    std::optional<SystemPortID> systemPortID;
    std::optional<int64_t> encapIndex;

    auto isAggregatePort = fields.port.isAggregatePort();
    auto switchId = isAggregatePort
        ? sw_->getScopeResolver()
              ->scope(sw_->getState(), fields.port)
              .switchId()
        : sw_->getScopeResolver()->scope(fields.port.phyPortID()).switchId();
    auto asic = sw_->getHwAsicTable()->getHwAsicIf(switchId);
    if (asic->isSupported(HwAsic::Feature::RESERVED_ENCAP_INDEX_RANGE)) {
      encapIndex = EncapIndexAllocator::getNextAvailableEncapIdx(state, *asic);
    }

    if (switchType == cfg::SwitchType::VOQ) {
      CHECK(!fields.port.isSystemPort());
      interfaceID = sw_->getState()->getInterfaceIDForPort(port);
      // SystemPortID is always same as the InterfaceID
      systemPortID = SystemPortID(interfaceID);
    } else {
      interfaceID = intfID_;
    }

    auto newState = state->clone();
    auto intfMap = newState->getInterfaces()->modify(&newState);
    auto intf = intfMap->getNode(interfaceID);
    auto intfNew = intf->clone();

    auto nbrTable = intf->getTable<NTable>();
    auto updatedNbrTable = nbrTable->toThrift();

    auto node = nbrTable->getEntryIf(fields.ip);
    if (node) {
      if (!force) {
        // don't replace an existing entry with a pending one unless
        // explicitly allowed
        return nullptr;
      }

      updatedNbrTable.erase(fields.ip.str());
    }

    auto nbrEntry = state::NeighborEntryFields();
    nbrEntry.ipaddress() = fields.ip.str();
    nbrEntry.mac() = fields.mac.toString();
    nbrEntry.interfaceId() = static_cast<uint32_t>(interfaceID);
    nbrEntry.state() =
        static_cast<state::NeighborState>(state::NeighborState::Pending);
    if (encapIndex.has_value()) {
      nbrEntry.encapIndex() = encapIndex.value();
    }

    if (switchType == cfg::SwitchType::VOQ) {
      // TODO: Support aggregate ports for VOQ switches
      CHECK(port.isPhysicalPort());
      CHECK(systemPortID.has_value());
      nbrEntry.portId() =
          PortDescriptor(SystemPortID(systemPortID.value())).toThrift();
      nbrEntry.isLocal() = fields.isLocal;
    } else {
      nbrEntry.portId() = ncachehelpers::getNeighborPortDescriptor(fields.port);
    }

    if (fields.classID.has_value()) {
      nbrEntry.classID() = fields.classID.value();
    }

    XLOG(DBG2) << "Adding pending entry for " << fields.ip << " on interface "
               << fields.interfaceID << " entry: "
               << apache::thrift::SimpleJSONSerializer::serialize<std::string>(
                      nbrEntry);

    updatedNbrTable.insert({fields.ip.str(), nbrEntry});
    intfNew->setNeighborTable<NTable>(updatedNbrTable);
    intfMap->updateNode(
        intfNew, sw_->getScopeResolver()->scope(intfNew, newState));

    return newState;
  };

  return updateFn;
}

template <typename NTable>
NeighborCacheImpl<NTable>::~NeighborCacheImpl() {}

template <typename NTable>
void NeighborCacheImpl<NTable>::repopulate(std::shared_ptr<NTable> table) {
  for (auto it = table->cbegin(); it != table->cend(); ++it) {
    auto entry = it->second;
    auto state = entry->isPending() ? NeighborEntryState::INCOMPLETE
                                    : NeighborEntryState::STALE;

    switch (entry->getType()) {
      case state::NeighborEntryType::DYNAMIC_ENTRY:
        setEntryInternal(
            EntryFields::fromThrift(entry->toThrift()),
            state,
            state::NeighborEntryType::DYNAMIC_ENTRY);
        break;
      case state::NeighborEntryType::STATIC_ENTRY:
        // We don't need to run Neighbor entry state machine for static
        // entries. Thus, skip adding to the neighbor cache
        XLOG(DBG2) << "Skip adding static entry to neighbor cache: "
                   << entry->getIP().str();
        break;
    }
  }
}

template <typename NTable>
void NeighborCacheImpl<NTable>::setEntry(
    AddressType ip,
    folly::MacAddress mac,
    PortDescriptor port,
    NeighborEntryState state) {
  auto entry = setEntryInternal(
      EntryFields(ip, mac, port, intfID_),
      state,
      state::NeighborEntryType::DYNAMIC_ENTRY);
  if (entry) {
    programEntry(entry);
  }
}

template <typename NTable>
void NeighborCacheImpl<NTable>::setExistingEntry(
    AddressType ip,
    folly::MacAddress mac,
    PortDescriptor port,
    NeighborEntryState state) {
  auto entry = setEntryInternal(
      EntryFields(ip, mac, port, intfID_),
      state,
      state::NeighborEntryType::DYNAMIC_ENTRY,
      false);
  if (entry) {
    // only program an entry if one exists
    programEntry(entry);
  }
}

template <typename NTable>
void NeighborCacheImpl<NTable>::updateEntryState(
    AddressType ip,
    NeighborEntryState state) {
  auto entry = getCacheEntry(ip);
  if (entry) {
    entry->updateState(state);
  }
}

template <typename NTable>
void NeighborCacheImpl<NTable>::updateEntryClassID(
    AddressType ip,
    std::optional<cfg::AclLookupClass> classID) {
  auto entry = getCacheEntry(ip);

  if (entry) {
    entry->updateClassID(classID);

    auto updateClassIDFn =
        [this, ip, classID](const std::shared_ptr<SwitchState>& state) {
          std::shared_ptr<SwitchState> newState{state};

          NTable* table;
          Interface* intf;
          Vlan* vlan;
          if (FLAGS_intf_nbr_tables) {
            intf = state->getInterfaces()->getNodeIf(intfID_).get();
            table = intf->template getNeighborTable<NTable>().get();
          } else {
            vlan = state->getVlans()->getNodeIf(vlanID_).get();
            table = vlan->template getNeighborTable<NTable>().get();
          }

          auto node = table->getNodeIf(ip.str());
          if (node) {
            auto fields = EntryFields(
                node->getIP(),
                node->getMac(),
                node->getPort(),
                node->getIntfID(),
                node->getState(),
                classID,
                node->getEncapIndex(),
                node->getIsLocal());

            if (FLAGS_intf_nbr_tables) {
              table = table->modify(&intf, &newState);
            } else {
              table = table->modify(&vlan, &newState);
            }

            table->updateEntry(fields);
          }

          return newState;
        };

    auto classIDStr = classID.has_value()
        ? folly::to<std::string>(static_cast<int>(classID.value()))
        : "None";
    sw_->updateState(
        folly::to<std::string>(
            "NeighborCache configure lookup classID: ",
            classIDStr,
            " for " + ip.str()),
        std::move(updateClassIDFn));
  }
}

template <typename NTable>
NeighborCacheEntry<NTable>* NeighborCacheImpl<NTable>::setEntryInternal(
    const EntryFields& fields,
    NeighborEntryState state,
    state::NeighborEntryType type,
    bool add) {
  auto entry = getCacheEntry(fields.ip);
  if (entry) {
    auto changed = !entry->fieldsMatch(fields);
    if (changed) {
      entry->updateFields(fields);
    }
    entry->updateState(state);
    return changed ? entry : nullptr;
  } else if (add) {
    auto to_store = std::make_shared<Entry>(fields, evb_, cache_, state, type);
    entry = to_store.get();
    setCacheEntry(std::move(to_store));
  }
  return entry;
}

template <typename NTable>
void NeighborCacheImpl<NTable>::setPendingEntry(
    AddressType ip,
    PortDescriptor port,
    bool force) {
  if (!force && getCacheEntry(ip)) {
    // only overwrite an existing entry with a pending entry if we say it is
    // ok with the 'force' parameter
    return;
  }

  auto entry = setEntryInternal(
      EntryFields(ip, intfID_, NeighborState::PENDING),
      NeighborEntryState::INCOMPLETE,
      state::NeighborEntryType::DYNAMIC_ENTRY,
      true);
  if (entry) {
    programPendingEntry(entry, port, force);
  }
}

template <typename NTable>
void NeighborCacheImpl<NTable>::processEntry(AddressType ip) {
  auto entry = getCacheEntry(ip);
  if (entry) {
    entry->process();
    if (entry->getState() == NeighborEntryState::EXPIRED) {
      flushEntry(ip);
    }
  }
}

template <typename NTable>
NeighborCacheEntry<NTable>* NeighborCacheImpl<NTable>::getCacheEntry(
    AddressType ip) const {
  auto it = entries_.find(ip);
  if (it != entries_.end()) {
    return it->second.get();
  }
  return nullptr;
}

template <typename NTable>
void NeighborCacheImpl<NTable>::setCacheEntry(std::shared_ptr<Entry> entry) {
  const auto& ip = entry->getIP();
  entries_[ip] = std::move(entry);
}

template <typename NTable>
bool NeighborCacheImpl<NTable>::removeEntry(AddressType ip) {
  auto it = entries_.find(ip);
  if (it == entries_.end()) {
    return false;
  }

  entries_.erase(it);

  return true;
}

template <typename NTable>
template <typename VlanOrIntfT>
bool NeighborCacheImpl<NTable>::flushEntryFromSwitchState(
    std::shared_ptr<SwitchState>* state,
    AddressType ip,
    VlanOrIntfT* vlanOrIntf) {
  auto* table = vlanOrIntf->template getNeighborTable<NTable>().get();
  const auto& entry = table->getNodeIf(ip.str());
  if (!entry) {
    return false;
  }

  table = table->modify(&vlanOrIntf, state);
  table->removeNode(ip.str());
  return true;
}

template <typename NTable>
bool NeighborCacheImpl<NTable>::flushEntryBlocking(AddressType ip) {
  bool flushed{false};
  flushEntry(ip, &flushed);
  return flushed;
}

template <typename NTable>
void NeighborCacheImpl<NTable>::flushEntry(AddressType ip, bool* flushed) {
  // remove from cache
  if (!removeEntry(ip)) {
    if (flushed) {
      *flushed = false;
    }
    return;
  }

  // flush from SwitchState
  auto updateFn = [this, ip, flushed](const std::shared_ptr<SwitchState>& state)
      -> std::shared_ptr<SwitchState> {
    std::shared_ptr<SwitchState> newState{state};

    bool flushedEntry;
    if (FLAGS_intf_nbr_tables) {
      auto* intf = newState->getInterfaces()->getNode(intfID_).get();
      flushedEntry = flushEntryFromSwitchState(&newState, ip, intf);
    } else {
      auto* vlan = newState->getVlans()->getNode(vlanID_).get();
      flushedEntry = flushEntryFromSwitchState(&newState, ip, vlan);
    }

    if (flushedEntry) {
      if (flushed) {
        *flushed = true;
      }
      return newState;
    }
    return nullptr;
  };

  if (flushed) {
    // need a blocking state update if the caller wants to know if an entry
    // was actually flushed
    sw_->updateStateBlocking("flush neighbor entry", std::move(updateFn));
  } else {
    sw_->updateState("remove neighbor entry: " + ip.str(), std::move(updateFn));
  }
}

template <typename NTable>
std::unique_ptr<typename NeighborCacheImpl<NTable>::EntryFields>
NeighborCacheImpl<NTable>::cloneEntryFields(AddressType ip) {
  auto entry = getCacheEntry(ip);
  if (entry) {
    return std::make_unique<EntryFields>(entry->getFields());
  }
  return nullptr;
}

template <typename NTable>
void NeighborCacheImpl<NTable>::portDown(PortDescriptor port) {
  for (auto item : entries_) {
    if (item.second->getPort() != port) {
      continue;
    }

    // TODO(aeckert): It would be nicer if we could just mark this
    // entry stale on port down so we don't need to unprogram the
    // entry (for fast port flaps).  However, we have seen packet
    // losses if we start forwarding packets on a port up event before
    // we receive a neighbor reply so it may not be worth leaving it
    // programmed. Also we need to notify the HwSwitch for ECMP expand
    // when the port comes back up and changing an entry from pending
    // to reachable is how we currently do this.
    setPendingEntry(item.second->getIP(), port, true);
  }
}

template <typename NTable>
void NeighborCacheImpl<NTable>::portFlushEntries(PortDescriptor port) {
  std::vector<AddressType> entriesToFlush;
  for (auto item : entries_) {
    if (item.second->getPort() != port) {
      continue;
    }
    entriesToFlush.push_back(item.second->getIP());
  }

  for (const auto& ip : entriesToFlush) {
    XLOG(DBG2) << "Flush neighbor entry " << ip.str() << " on port " << port;
    flushEntry(ip);
  }
}

template <typename NTable>
template <typename NeighborEntryThrift>
std::list<NeighborEntryThrift> NeighborCacheImpl<NTable>::getCacheData() const {
  std::list<NeighborEntryThrift> thriftEntries;
  for (const auto& item : entries_) {
    NeighborEntryThrift thriftEntry;
    item.second->populateThriftEntry(thriftEntry);
    thriftEntries.push_back(thriftEntry);
  }
  return thriftEntries;
}

template <typename NTable>
template <typename NeighborEntryThrift>
std::optional<NeighborEntryThrift> NeighborCacheImpl<NTable>::getCacheData(
    AddressType ip) const {
  std::optional<NeighborEntryThrift> cachedNeighborEntry;
  auto entry = getCacheEntry(ip);
  if (entry) {
    NeighborEntryThrift thriftEntry;
    entry->populateThriftEntry(thriftEntry);
    cachedNeighborEntry.assign(thriftEntry);
  }
  return cachedNeighborEntry;
}

} // namespace facebook::fboss
