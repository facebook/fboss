// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/hw/bcm/BcmNextHop.h"

#include "fboss/agent/hw/bcm/BcmEgressManager.h"
#include "fboss/agent/hw/bcm/BcmHost.h"
#include "fboss/agent/hw/bcm/BcmHostKey.h"
#include "fboss/agent/hw/bcm/BcmIntf.h"
#include "fboss/agent/hw/bcm/BcmLabeledEgress.h"
#include "fboss/agent/hw/bcm/BcmLabeledTunnelEgress.h"
#include "fboss/agent/hw/bcm/BcmMultiPathNextHop.h"
#include "fboss/agent/hw/bcm/BcmPortTable.h"
#include "fboss/agent/hw/bcm/BcmSwitch.h"
#include "fboss/agent/hw/bcm/BcmTrunkTable.h"

namespace {
template <typename KetT>
std::string nextHopKeyStr(const KetT& key) {
  return key.str();
}

std::string nextHopKeyStr(const facebook::fboss::BcmMultiPathNextHopKey& key) {
  std::string str = folly::to<std::string>("vrf:", key.first, "->{");
  for (const auto& nhop : key.second) {
    str = folly::to<std::string>(nhop.str(), ",");
  }
  str += "}";
  return str;
}
} // namespace
namespace facebook::fboss {

bcm_if_t BcmL3NextHop::getEgressId() const {
  return host_->getEgressId();
}

void BcmL3NextHop::programToCPU(bcm_if_t intf) {
  host_->programToCPU(intf);
}

bool BcmL3NextHop::isProgrammed() const {
  return getEgressId() != BcmEgressBase::INVALID;
}

bcm_if_t BcmMplsNextHop::getEgressId() const {
  return mplsEgress_->getID();
}

void BcmMplsNextHop::programToCPU(bcm_if_t /*intf*/) {
  /* this is called only from BcmMultiPathNextHop, or multipath next hop, and is
  called only if, MPLS nexthop is "not programmed". this should never happen
  because constructor of MPLS nexthop creates egress and programs it either to
  drop, to cpu or to appropriate port/trunk.

  So call to this should never happen from BcmMultiPathNextHop and MPLS nexthop
  would always be programmed when it's referred in multipath next hop.
  */
  CHECK(false);
}

bool BcmMplsNextHop::isProgrammed() const {
  return getEgressId() != BcmEgressBase::INVALID;
}

BcmL3NextHop::BcmL3NextHop(BcmSwitch* hw, BcmHostKey key)
    : hw_(hw), key_(std::move(key)) {
  host_ = hw_->writableHostTable()->refOrEmplace(key_);
}

BcmMplsNextHop::BcmMplsNextHop(BcmSwitch* hw, BcmLabeledHostKey key)
    : hw_(hw), key_(std::move(key)) {
  program(BcmHostKey(key_.getVrf(), key_.addr(), key_.intfID()));
}

void BcmMplsNextHop::program(BcmHostKey bcmHostKey) {
  CHECK_EQ(bcmHostKey, getBcmHostKey());

  BcmHost* bcmHostEntry = hw_->getNeighborTable()->getNeighborIf(bcmHostKey);

  if (!mplsEgress_) {
    /* no egress exists, create one */
    mplsEgress_ = createEgress();
  }

  bcm_gport_t oldGport = getGPort();
  bcm_gport_t newGport = BcmPort::asGPort(0);
  bcm_port_t newPort{0};
  bcm_trunk_t newTrunk{BcmTrunk::INVALID};

  BcmEcmpEgress::Action ecmpAction;

  auto bcmIntfId = hw_->getIntfTable()->getBcmIntf(key_.intf())->getBcmIfId();

  if (!bcmHostEntry || bcmHostEntry->isProgrammedToDrop()) {
    /* no known l3 next hop for this labeled next host program to drop */
    XLOG(DBG2) << "programming " << key_.str() << " to drop";
    mplsEgress_->programToDrop(bcmIntfId, key_.getVrf(), key_.addr());
    ecmpAction = BcmEcmpEgress::Action::SHRINK;
  } else if (bcmHostEntry->isProgrammedToCPU()) {
    /* l3 next hop for this labeled host is known but l3 egress is unresolved */
    /* program to CPU */
    XLOG(DBG2) << "programming " << key_.str() << " to CPU";
    mplsEgress_->programToCPU(bcmIntfId, key_.getVrf(), key_.addr());
    ecmpAction = BcmEcmpEgress::Action::SHRINK;
  } else {
    /* l3 next hop for this labeled host is known and l3 egress is resolved */
    /* program to next hop */
    auto bcmHostPortDesc = bcmHostEntry->getEgressPortDescriptor();
    CHECK(bcmHostPortDesc.has_value());
    auto* bcmHostEgress = bcmHostEntry->getEgress();
    auto mac = bcmHostEgress->getMac();
    if (bcmHostPortDesc->isAggregatePort()) {
      // program over trunk
      newTrunk = bcmHostPortDesc->aggPortID();
      newGport = BcmTrunk::asGPort(newTrunk);
      XLOG(DBG2) << "programming " << key_.str() << " to trunk " << newTrunk;
      mplsEgress_->programToTrunk(
          bcmIntfId, bcmHostKey.getVrf(), bcmHostKey.addr(), mac, newTrunk);
    } else {
      // program over port
      newPort = bcmHostPortDesc->phyPortID();
      newGport = BcmPort::asGPort(newPort);
      XLOG(DBG2) << "programming " << key_.str() << " to port " << newPort;
      mplsEgress_->programToPort(
          bcmIntfId, bcmHostKey.getVrf(), bcmHostKey.addr(), mac, newPort);
    }
    ecmpAction = BcmEcmpEgress::Action::EXPAND;
  }

  if (ecmpAction == BcmEcmpEgress::Action::EXPAND) {
    hw_->writableEgressManager()->resolved(mplsEgress_->getID());
  } else if (ecmpAction == BcmEcmpEgress::Action::SHRINK) {
    hw_->writableEgressManager()->unresolved(mplsEgress_->getID());
  }
  // update egress mapping and  multipath resolution
  hw_->writableEgressManager()->updatePortToEgressMapping(
      mplsEgress_->getID(), oldGport, newGport);
  hw_->writableMultiPathNextHopTable()->egressResolutionChangedHwLocked(
      mplsEgress_->getID(), ecmpAction);

  newTrunk != BcmTrunk::INVALID ? setTrunk(newTrunk) : setPort(newPort);
}

BcmMplsNextHop::~BcmMplsNextHop() {
  if (!mplsEgress_) {
    return;
  }
  if (hw_->getEgressManager()->isResolved(mplsEgress_->getID())) {
    hw_->writableEgressManager()->unresolved(mplsEgress_->getID());
  }
  // This host mapping just went away, update the port -> egress id mapping
  bool isPortOrTrunkSet = egressPort_.has_value();
  bcm_gport_t gPort = getGPort();
  hw_->writableEgressManager()->updatePortToEgressMapping(
      mplsEgress_->getID(), gPort, BcmPort::asGPort(0));
  hw_->writableMultiPathNextHopTable()->egressResolutionChangedHwLocked(
      mplsEgress_->getID(),
      isPortOrTrunkSet ? BcmEcmpEgress::Action::SHRINK
                       : BcmEcmpEgress::Action::SKIP);
}

std::unique_ptr<BcmEgress> BcmMplsNextHop::createEgress() {
  CHECK(key_.hasLabel());

  if (!key_.needsMplsTunnel()) {
    // create egress with label
    return std::make_unique<BcmLabeledEgress>(hw_, key_.getLabel());
  }
  // create egress with tunnel
  auto* intf = hw_->getIntfTable()->getBcmIntf(key_.intfID().value());
  return std::make_unique<BcmLabeledTunnelEgress>(
      hw_, key_.getLabel(), intf->getBcmIfId(), key_.tunnelLabelStack());
}

bcm_gport_t BcmMplsNextHop::getGPort() {
  bcm_gport_t gport = BcmPort::asGPort(0);
  if (egressPort_) {
    switch (egressPort_->type()) {
      case PortDescriptor::PortType::AGGREGATE:
        gport = BcmTrunk::asGPort(
            hw_->getTrunkTable()->getBcmTrunkId(egressPort_->aggPortID()));
        break;
      case PortDescriptor::PortType::PHYSICAL:
        gport = BcmPort::asGPort(
            hw_->getPortTable()->getBcmPortId(egressPort_->phyPortID()));
        break;
    }
  }
  return gport;
}

void BcmMplsNextHop::setPort(bcm_port_t port) {
  egressPort_ = PortDescriptor(hw_->getPortTable()->getPortId(port));
}

void BcmMplsNextHop::setTrunk(bcm_trunk_t trunk) {
  egressPort_ = PortDescriptor(hw_->getTrunkTable()->getAggregatePortId(trunk));
}

BcmLabeledEgress* BcmMplsNextHop::getBcmLabeledEgress() const {
  CHECK(!key_.needsMplsTunnel());
  return static_cast<BcmLabeledEgress*>(mplsEgress_.get());
}

BcmLabeledTunnelEgress* BcmMplsNextHop::getBcmLabeledTunnelEgress() const {
  CHECK(key_.needsMplsTunnel());
  return static_cast<BcmLabeledTunnelEgress*>(mplsEgress_.get());
}

template <typename NextHopKeyT, typename NextHopT>
const NextHopT* BcmNextHopTable<NextHopKeyT, NextHopT>::getNextHop(
    const NextHopKeyT& key) const {
  auto rv = getNextHopIf(key);
  if (!rv) {
    throw FbossError("nexthop ", nextHopKeyStr(key), " not found");
  }
  return rv;
}

template <typename NextHopKeyT, typename NextHopT>
std::shared_ptr<NextHopT>
BcmNextHopTable<NextHopKeyT, NextHopT>::referenceOrEmplaceNextHop(
    const NextHopKeyT& key) {
  auto rv = nexthops_.refOrEmplace(key, hw_, key);
  if (rv.second) {
    XLOG(DBG3) << "inserted reference to next hop " << nextHopKeyStr(key);
  } else {
    XLOG(DBG3) << "accessed reference to next hop " << nextHopKeyStr(key);
  }
  return rv.first;
}

template class BcmNextHopTable<BcmHostKey, BcmL3NextHop>;
template class BcmNextHopTable<BcmLabeledHostKey, BcmMplsNextHop>;
template class BcmNextHopTable<BcmMultiPathNextHopKey, BcmMultiPathNextHop>;

} // namespace facebook::fboss
