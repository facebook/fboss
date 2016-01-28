/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "fboss/agent/ndp/IPv6RouteAdvertiser.h"

#include <netinet/icmp6.h>
#include <folly/IPAddressV6.h>
#include <folly/MacAddress.h>
#include "fboss/agent/FbossError.h"
#include "fboss/agent/SwSwitch.h"
#include "fboss/agent/TxPacket.h"
#include "fboss/agent/packet/Ethertype.h"
#include "fboss/agent/packet/IPv6Hdr.h"
#include "fboss/agent/packet/ICMPHdr.h"
#include "fboss/agent/packet/PktUtil.h"
#include "fboss/agent/state/Interface.h"
#include "fboss/agent/state/SwitchState.h"
#include "fboss/agent/state/Vlan.h"
#include "fboss/agent/state/VlanMap.h"

using folly::IPAddressV6;
using folly::MacAddress;
using folly::io::Cursor;
using folly::IOBuf;
using folly::io::RWPrivateCursor;

namespace {

uint32_t getAdvertisementPacketBodySize(uint32_t items_count)  {
  uint32_t bodyLength =
    4 + // hop limit, flags, lifetime
    4 + // reachable timer
    4 + // retrans timer
    8 + // src MAC option
    8 + // MTU option
    (32 * items_count); // prefix options

  return bodyLength;
}

template<typename F>
void foreachAddrToAdvertise(const facebook::fboss::Interface* intf, F f) {
  for (const auto& addr : intf->getAddresses()) {
    if (!addr.first.isV6()) {
      continue;
    }
    uint8_t mask = addr.second;
    // For IPs that are configured with a 128-bit mask, don't bother including
    // them in the advertisement, since no-one else besides us can belong to
    // this subnet.
    if (mask == 128) {
      continue;
    }
    f(addr);
  }
}

}

namespace facebook { namespace fboss {

/*
 * IPv6RAImpl is the class that actually handles sending out the RA packets.
 *
 * This uses AsyncTimeout to receive timeout notifications in the SwSwitch's
 * background event thread, and send out RA packets every time the timer fires.
 */
class IPv6RAImpl : private folly::AsyncTimeout {
 public:
  IPv6RAImpl(SwSwitch* sw,
             const SwitchState* state,
             const Interface* intf);

  SwSwitch* getSw() const {
    return sw_;
  }

  static void start(IPv6RAImpl* ra);
  static void stop(IPv6RAImpl* ra);

 private:
  // Forbidden copy constructor and assignment operator
  IPv6RAImpl(IPv6RAImpl const &) = delete;
  IPv6RAImpl& operator=(IPv6RAImpl const &) = delete;

  void timeoutExpired() noexcept override {
    sendRouteAdvertisement();
    scheduleTimeout(interval_);
  }

  void initPacket(const Interface* intf);
  void sendRouteAdvertisement();

  std::chrono::milliseconds interval_;
  folly::IOBuf buf_;
  SwSwitch* const sw_{nullptr};
};

IPv6RAImpl::IPv6RAImpl(SwSwitch* sw,
                       const SwitchState* state,
                       const Interface* intf)
  : AsyncTimeout(sw->getBackgroundEVB()),
    sw_(sw) {
  std::chrono::seconds raInterval(
      intf->getNdpConfig().routerAdvertisementSeconds);
  interval_ = raInterval;
  initPacket(intf);
}

void IPv6RAImpl::start(IPv6RAImpl* ra) {
  ra->scheduleTimeout(ra->interval_);
}

void IPv6RAImpl::stop(IPv6RAImpl* ra) {
  /*
   * Just before going down we send one last route
   * advertisement to avoid RA's timing out while
   * controller is restarted
   */
  ra->sendRouteAdvertisement();
  delete ra;
}

void IPv6RAImpl::initPacket(const Interface* intf) {
  auto totalLength = IPv6RouteAdvertiser::getPacketSize(intf);
  buf_ = IOBuf(IOBuf::CREATE, totalLength);
  buf_.append(totalLength);
  RWPrivateCursor cursor(&buf_);
  IPv6RouteAdvertiser::createAdvertisementPacket(
    intf, &cursor, MacAddress("33:33:00:00:00:01"), IPAddressV6("ff02::1"));
}

void IPv6RAImpl::sendRouteAdvertisement() {
  VLOG(5) << "sending route advertisement:\n" <<
    PktUtil::hexDump(Cursor(&buf_));

  // Allocate a new packet, and copy our data into it.
  //
  // Note that we unfortunately do need to perform a buffer copy here.
  // The TxPacket is required to use DMA memory for its buffer, so we can't do
  // a simple clone.
  //
  // TODO: In the future it would be nice to support allocating buf_ in a DMA
  // buffer so that we really can just clone a reference to it here, rather
  // than doing a copy.
  uint32_t pktLen = buf_.length();
  auto pkt = sw_->allocatePacket(pktLen);
  RWPrivateCursor cursor(pkt->buf());
  cursor.push(buf_.data(), buf_.length());

  sw_->sendPacketSwitched(std::move(pkt));
}

IPv6RouteAdvertiser::IPv6RouteAdvertiser(SwSwitch* sw,
                                         const SwitchState* state,
                                         const Interface* intf) {
  adv_ = new IPv6RAImpl(sw, state, intf);
  bool ret = sw->getBackgroundEVB()->runInEventBaseThread(
      IPv6RAImpl::start, adv_);
  if (!ret) {
    delete adv_;
    adv_ = nullptr;
    throw FbossError("failed to start IPv6 route advertiser for interface ",
                     intf->getID());

  }
}

IPv6RouteAdvertiser::IPv6RouteAdvertiser(IPv6RouteAdvertiser&& other) noexcept
  : adv_(other.adv_) {
    other.adv_ = nullptr;
}

IPv6RouteAdvertiser::~IPv6RouteAdvertiser() {
  if (!adv_) {
    return;
  }
  bool ret = adv_->getSw()->getBackgroundEVB()->runInEventBaseThread(
      IPv6RAImpl::stop, adv_);
  if (!ret) {
    LOG(ERROR) << "failed to stop IPv6 route advertiser";
  }
}

IPv6RouteAdvertiser& IPv6RouteAdvertiser::operator=(
    IPv6RouteAdvertiser&& other) noexcept {
  adv_ = other.adv_;
  other.adv_ = nullptr;
  return *this;
}

/* static */ uint32_t IPv6RouteAdvertiser::getPacketSize(
    const Interface* intf) {
  uint32_t count = 0;
  foreachAddrToAdvertise(intf, [&](
    const std::pair<folly::IPAddress, uint8_t> &) {
      ++count;
    });

  auto bodyLength = getAdvertisementPacketBodySize(count);

  return ICMPHdr::computeTotalLengthV6(bodyLength);
}

/* static */ void IPv6RouteAdvertiser::createAdvertisementPacket(
    const Interface* intf,
    folly::io::RWPrivateCursor* cursor,
    folly::MacAddress dstMac,
    const folly::IPAddressV6& dstIP) {
  const auto* ndpConfig = &intf->getNdpConfig();

  // Settings
  uint8_t hopLimit = ndpConfig->curHopLimit;
  // Set managed and other bits in router advertisements.
  // These bits control whether address and other information
  // (e.g. where to grab the image) is available via DHCP.
  uint8_t flags = 0;
  if (ndpConfig->routerAdvertisementManagedBit) {
    flags |= ND_RA_FLAG_MANAGED;
  }
  if (ndpConfig->routerAdvertisementOtherBit) {
    flags |= ND_RA_FLAG_OTHER;
  }
  std::chrono::seconds lifetime(ndpConfig->routerLifetime);
  std::chrono::seconds reachableTimer(0);
  std::chrono::seconds retransTimer(0);
  uint32_t prefixValidLifetime = ndpConfig->prefixValidLifetimeSeconds;
  uint32_t prefixPreferredLifetime = ndpConfig->prefixPreferredLifetimeSeconds;

  // Build the list of prefixes to advertise
  typedef std::pair<IPAddressV6, uint8_t> Prefix;
  uint32_t mtu = intf->getMtu();
  std::set<Prefix> prefixes;
  foreachAddrToAdvertise(intf, [&](
    const std::pair<folly::IPAddress, uint8_t> & addr) {
      uint8_t mask = addr.second;
      prefixes.emplace(addr.first.asV6().mask(mask), mask);
    });

  auto serializeBody = [&](RWPrivateCursor* cursor) {
    cursor->writeBE<uint8_t>(hopLimit);
    cursor->writeBE<uint8_t>(flags);
    cursor->writeBE<uint16_t>(lifetime.count());
    cursor->writeBE<uint32_t>(reachableTimer.count());
    cursor->writeBE<uint32_t>(retransTimer.count());

    // Source MAC option
    cursor->writeBE<uint8_t>(1); // Option type (src link-layer address)
    cursor->writeBE<uint8_t>(1); // Option length = 1 (x8)
    cursor->push(intf->getMac().bytes(), MacAddress::SIZE);

    // Prefix options
    for (const auto& prefix : prefixes) {
      cursor->writeBE<uint8_t>(3); // Option type (prefix information)
      cursor->writeBE<uint8_t>(4); // Option length = 4 (x8)
      cursor->writeBE<uint8_t>(prefix.second);
      uint8_t prefixFlags = 0xc0; // on link, autonomous address configuration
      cursor->writeBE<uint8_t>(prefixFlags);
      cursor->writeBE<uint32_t>(prefixValidLifetime);
      cursor->writeBE<uint32_t>(prefixPreferredLifetime);
      cursor->writeBE<uint32_t>(0); // reserved
      cursor->push(prefix.first.bytes(), IPAddressV6::byteCount());
    }

    // MTU option
    cursor->writeBE<uint8_t>(5); // Option type (MTU)
    cursor->writeBE<uint8_t>(1); // Option length = 1 (x8)
    cursor->writeBE<uint16_t>(0); // Reserved
    cursor->writeBE<uint32_t>(mtu);
  };

  auto bodyLength = getAdvertisementPacketBodySize(prefixes.size());

  IPAddressV6 srcIP(IPAddressV6::LINK_LOCAL, intf->getMac());
  IPv6Hdr ipv6(srcIP, dstIP);
  ipv6.trafficClass = 0xe0; // CS7 precedence (network control)
  ipv6.payloadLength = ICMPHdr::SIZE + bodyLength;
  ipv6.nextHeader = IP_PROTO_IPV6_ICMP;
  ipv6.hopLimit = 255;

  ICMPHdr icmp6(ICMPV6_TYPE_NDP_ROUTER_ADVERTISEMENT, 0, 0);

  icmp6.serializeFullPacket(cursor, dstMac, intf->getMac(), intf->getVlanID(),
                            ipv6, bodyLength, serializeBody);
}

}} // facebook::fboss
