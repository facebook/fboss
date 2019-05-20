// Copyright 2004-present Facebook. All Rights Reserved.

#include "fboss/agent/hw/bcm/BcmNextHop.h"

#include "fboss/agent/hw/bcm/BcmHostKey.h"
#include "fboss/agent/hw/bcm/BcmIntf.h"
#include "fboss/agent/hw/bcm/BcmLabeledEgress.h"
#include "fboss/agent/hw/bcm/BcmLabeledTunnelEgress.h"
#include "fboss/agent/hw/bcm/BcmPortTable.h"
#include "fboss/agent/hw/bcm/BcmSwitch.h"
#include "fboss/agent/hw/bcm/BcmTrunkTable.h"

namespace facebook {
namespace fboss {

opennsl_if_t BcmL3NextHop::getEgressId() const {
  return hostReference_->getEgressId();
}

void BcmL3NextHop::programToCPU(opennsl_if_t intf) {
  auto* host = hostReference_->getBcmHost();
  host->programToCPU(intf);
}

bool BcmL3NextHop::isProgrammed() const {
  return getEgressId() != BcmEgressBase::INVALID;
}

opennsl_if_t BcmMplsNextHop::getEgressId() const {
  return mplsEgress_->getID();
}

void BcmMplsNextHop::programToCPU(opennsl_if_t /*intf*/) {
  /* this is called only from BcmEcmpHost, or multipath next hop, and is called
  only if, MPLS nexthop is "not programmed". this should never happen because
  constructor of MPLS nexthop creates egress and programs it either to drop, to
  cpu or to appropriate port/trunk.

  So call to this should never happen from BcmEcmpHost and MPLS nexthop would
  always be programmed when it's referred in multipath next hop.
  */
  CHECK(false);
}

bool BcmMplsNextHop::isProgrammed() const {
  return getEgressId() != BcmEgressBase::INVALID;
}

BcmL3NextHop::BcmL3NextHop(BcmSwitch* hw, BcmHostKey key)
    : hw_(hw), key_(std::move(key)) {
  hostReference_ = BcmHostReference::get(hw_, key_);
  hostReference_->getEgressId(); // load reference
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

  opennsl_gport_t oldGport = getGPort();
  opennsl_gport_t newGport = BcmPort::asGPort(0);
  opennsl_port_t newPort{0};
  opennsl_trunk_t newTrunk{BcmTrunk::INVALID};

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
    auto bcmHostPortDesc = bcmHostEntry->portDescriptor();
    auto bcmHostEgressId = bcmHostEntry->getEgressId();
    auto* bcmHostEgress =
        hw_->getHostTable()->getEgressObjectIf(bcmHostEgressId);
    auto mac = bcmHostEgress->getMac();
    if (bcmHostPortDesc.isAggregatePort()) {
      // program over trunk
      newTrunk =
          hw_->getTrunkTable()->getBcmTrunkId(bcmHostPortDesc.aggPortID());
      newGport = BcmTrunk::asGPort(newTrunk);
      XLOG(DBG2) << "programming " << key_.str() << " to trunk " << newTrunk;
      mplsEgress_->programToTrunk(
          bcmIntfId, bcmHostKey.getVrf(), bcmHostKey.addr(), mac, newTrunk);
    } else {
      // program over port
      newPort = hw_->getPortTable()->getBcmPortId(bcmHostPortDesc.phyPortID());
      newGport = BcmPort::asGPort(newPort);
      XLOG(DBG2) << "programming " << key_.str() << " to port " << newPort;
      mplsEgress_->programToPort(
          bcmIntfId, bcmHostKey.getVrf(), bcmHostKey.addr(), mac, newPort);
    }
    ecmpAction = BcmEcmpEgress::Action::EXPAND;
  }

  if (ecmpAction == BcmEcmpEgress::Action::EXPAND) {
    hw_->writableHostTable()->resolved(mplsEgress_->getID());
  } else if (ecmpAction == BcmEcmpEgress::Action::SHRINK) {
    hw_->writableHostTable()->unresolved(mplsEgress_->getID());
  }
  // update egress mapping and  multipath resolution
  hw_->writableHostTable()->updatePortToEgressMapping(
      mplsEgress_->getID(), oldGport, newGport);
  hw_->writableHostTable()->egressResolutionChangedHwLocked(
      mplsEgress_->getID(), ecmpAction);

  newTrunk != BcmTrunk::INVALID ? setTrunk(newTrunk) : setPort(newPort);
}

BcmMplsNextHop::~BcmMplsNextHop() {
  if (!mplsEgress_) {
    return;
  }
  if (hw_->getHostTable()->isResolved(mplsEgress_->getID())) {
    hw_->writableHostTable()->unresolved(mplsEgress_->getID());
  }
  // This host mapping just went away, update the port -> egress id mapping
  bool isPortOrTrunkSet = egressPort_.hasValue();
  opennsl_gport_t gPort = getGPort();
  hw_->writableHostTable()->updatePortToEgressMapping(
      mplsEgress_->getID(), gPort, BcmPort::asGPort(0));
  hw_->writableHostTable()->egressResolutionChangedHwLocked(
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

opennsl_gport_t BcmMplsNextHop::getGPort() {
  opennsl_gport_t gport = BcmPort::asGPort(0);
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

void BcmMplsNextHop::setPort(opennsl_port_t port) {
  egressPort_.assign(PortDescriptor(hw_->getPortTable()->getPortId(port)));
}

void BcmMplsNextHop::setTrunk(opennsl_trunk_t trunk) {
  egressPort_.assign(
      PortDescriptor(hw_->getTrunkTable()->getAggregatePortId(trunk)));
}

} // namespace fboss
} // namespace facebook
