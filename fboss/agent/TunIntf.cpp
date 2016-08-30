/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "TunIntf.h"

extern "C" {
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <linux/if.h>
#include <linux/if_tun.h>
#include <netlink/route/link.h>
}

#include "fboss/agent/RxPacket.h"
#include "fboss/agent/SwSwitch.h"
#include "fboss/agent/SysError.h"
#include "fboss/agent/TxPacket.h"
#include "fboss/agent/packet/EthHdr.h"
#include <folly/io/async/EventBase.h>
#include <folly/io/async/EventHandler.h>

namespace facebook { namespace fboss {

namespace {

const std::string kTunIntfPrefix = "fboss";
const std::string kTunDev = "/dev/net/tun";

// Max packets to be processed which are received from host
const int kMaxSentOneTime = 16;

} // anonymous namespace

TunIntf::TunIntf(
    SwSwitch *sw,
    folly::EventBase *evb,
    InterfaceID ifID,
    int ifIndex,
    int mtu)
    : folly::EventHandler(evb),
      sw_(sw),
      name_(createTunIntfName(ifID)),
      ifID_(ifID),
      ifIndex_(ifIndex),
      mtu_(mtu) {
  DCHECK(sw) << "NULL pointer to SwSwitch.";
  DCHECK(evb) << "NULL pointer to EventBase";

  openFD();
  SCOPE_FAIL {
    closeFD();
  };

  LOG(INFO) << "Added interface " << name_ << " with fd " << fd_
            << " @ index " << ifIndex_;
}

TunIntf::TunIntf(
    SwSwitch *sw,
    folly::EventBase *evb,
    InterfaceID ifID,
    const Interface::Addresses& addr,
    int mtu)
    : folly::EventHandler(evb),
      sw_(sw),
      name_(createTunIntfName(ifID)),
      ifID_(ifID),
      addrs_(addr),
      mtu_(mtu) {
  DCHECK(sw) << "NULL pointer to SwSwitch.";
  DCHECK(evb) << "NULL pointer to EventBase";

  // Open Tun interface FD for socket-IO
  openFD();
  SCOPE_FAIL {
    closeFD();
  };

  // Make the Tun interface persistent, so that the network sessions from the
  // application (i.e. BGP)  will not be reset if controller restarts
  auto ret = ioctl(fd_, TUNSETPERSIST, 1);
  sysCheckError(ret, "Failed to set persist interface ", name_);

  // TODO: if needed, we can adjust send buffer size, TUNSETSNDBUF
  auto sock = nl_socket_alloc();
  if (!sock) {
    throw SysError(errno, "failed to open libnl socket");
  }
  SCOPE_EXIT { nl_socket_free(sock); };

  // Connect netlink socket.
  ret = nl_connect(sock, NETLINK_ROUTE);
  sysCheckError(ret, "failed to connect", nl_geterror(ret));
  SCOPE_EXIT { nl_close(sock); };

  // Allocate cache
  nl_cache *cache = nullptr;
  ret = rtnl_link_alloc_cache(sock, AF_UNSPEC, &cache);
  sysCheckError(ret, "failed to get all links: error: ", nl_geterror(ret));
  SCOPE_EXIT { nl_cache_free(cache); };

  // Extract ifIndex
  ifIndex_ = rtnl_link_name2i(cache, name_.c_str());
  if (ifIndex_ <= 0) {
    FbossError("Got invalid value ", ifIndex_, " for Tun interface ", name_);
  }

  LOG(INFO) << "Created interface " << name_ << " with fd " << fd_
            << " @ index " << ifIndex_;
}

TunIntf::~TunIntf() {
  stop();

  // We must have a valid fd to TunIntf
  CHECK_NE(fd_, -1);

  // Delete interface if need be
  if (toDelete_) {
    auto ret = ioctl(fd_, TUNSETPERSIST, 0);
    sysLogError(ret, "Failed to unset persist interface ", name_);
  }

  // Close FD. This will delete the interface if TUNSETPERSIST is not on
  closeFD();
  LOG(INFO) << (toDelete_ ? "Delete" : "Detach") << " interface " << name_;
}

bool TunIntf::isTunIntfName(std::string const& ifName) {
  // Special case to handle old front0 interface.
  // XXX: Delete this case after 6 months (atleast one full rollout). 08-27-2016
  if (ifName == "front0") {
    return true;
  }

  return ifName.find(kTunIntfPrefix) == 0;
}

std::string TunIntf::createTunIntfName(InterfaceID ifID) {
  // Special case to handle old front0 interface.
  // XXX: Delete this case after 6 months (atleast one full rollout). 08-27-2016
  if (ifID == InterfaceID(0)) {
    return "front0";
  }

  return folly::sformat("{}{}", kTunIntfPrefix, folly::to<std::string>(ifID));
}

InterfaceID TunIntf::getIDFromTunIntfName(std::string const& ifName) {
  if (not isTunIntfName(ifName)) {
    throw FbossError(ifName, " is not a valid tun interface");
  }

  // Special case to handle old front0 interface.
  // XXX: Delete this case after 6 months (atleast one full rollout). 08-27-2016
  if (ifName == "front0") {
    return InterfaceID(0);
  }

  return InterfaceID(atoi(ifName.substr(kTunIntfPrefix.size()).c_str()));
}

void TunIntf::stop() {
  unregisterHandler();
}

void TunIntf::start() {
  if (fd_ != -1 && !isHandlerRegistered()) {
    changeHandlerFD(fd_);
    registerHandler(folly::EventHandler::READ | folly::EventHandler::PERSIST);
  }
}

void TunIntf::openFD() {
  fd_ = open(kTunDev.c_str(), O_RDWR);
  sysCheckError(fd_, "Cannot open ", kTunDev.c_str());
  SCOPE_FAIL {
    closeFD();
  };

  struct ifreq ifr;
  memset(&ifr, 0, sizeof(ifr));
  // Flags: IFF_TUN   - TUN device (no Ethernet headers)
  //        IFF_NO_PI - Do not provide packet information
  ifr.ifr_flags = IFF_TUN|IFF_NO_PI;
  bzero(ifr.ifr_name, sizeof(ifr.ifr_name));
  size_t len = std::min(name_.size(), sizeof(ifr.ifr_name));
  memmove(ifr.ifr_name, name_.c_str(), len);
  auto ret = ioctl(fd_, TUNSETIFF, (void *) &ifr);
  sysCheckError(ret, "Failed to create/attach interface ", name_);

  // Set configured MTU
  setMtu(mtu_);

  // make fd non-blocking
  auto flags = fcntl(fd_, F_GETFL);
  sysCheckError(flags, "Failed to get flags from fd ", fd_);
  flags |= O_NONBLOCK;
  ret = fcntl(fd_, F_SETFL, flags);
  sysCheckError(ret, "Failed to set non-blocking flags ", flags,
                " to fd ", fd_);
  flags = fcntl(fd_, F_GETFD);
  sysCheckError(flags, "Failed to get flags from fd ", fd_);
  flags |= FD_CLOEXEC;
  ret = fcntl(fd_, F_SETFD, flags);
  sysCheckError(ret, "Failed to set close-on-exec flags ", flags,
                " to fd ", fd_);

  LOG(INFO) << "Create/attach to tun interface " << name_ << " @ fd " << fd_;
}

void TunIntf::closeFD() noexcept {
  auto ret = close(fd_);
  sysLogError(ret, "Failed to close fd ", fd_, " for interface ", name_);
  if (ret == 0) {
    LOG(INFO) << "Closed fd " << fd_ << " for interface " << name_;
    fd_ = -1;
  }
}

void TunIntf::addAddress(const folly::IPAddress& addr, uint8_t mask) {
  auto ret = addrs_.emplace(addr, mask);
  if (!ret.second) {
    throw FbossError("Duplication interface address ", addr, "/",
                     static_cast<int>(mask), " for interface ", name_,
                     " @ index", ifIndex_);
  }
  VLOG(3) << "Added address " << addr.str() << "/"
          << static_cast<int>(mask) << " to interface " << name_
          << " @ index " << ifIndex_;
}

void TunIntf::setMtu(int mtu) {
  mtu_ = mtu;
  auto sock = socket(PF_INET, SOCK_DGRAM, 0);
  sysCheckError(sock, "Failed to open socket");

  struct ifreq ifr;
  size_t len = std::min(name_.size(), sizeof(ifr.ifr_name));
  memset(&ifr, 0, sizeof(ifr));
  memmove(ifr.ifr_name, name_.c_str(), len);
  ifr.ifr_mtu = mtu_;
  auto ret = ioctl(sock, SIOCSIFMTU, (void*)&ifr);
  close(sock);
  sysCheckError(ret, "Failed to set MTU ", ifr.ifr_mtu,
                " to fd ", fd_, " errno = ", errno);
  VLOG(3) << "Set tun " << name_ << " MTU to " << mtu;
}

void TunIntf::handlerReady(uint16_t events) noexcept {
  CHECK(fd_ != -1);

  // Since this is L3 packet size, we should also reserve some space for L2
  // header, which is 18 bytes (including one vlan tag)
  int sent = 0;
  int dropped = 0;
  uint64_t bytes = 0;
  bool fdFail = false;
  try {
    while (sent + dropped < kMaxSentOneTime) {
      std::unique_ptr<TxPacket> pkt;
      pkt = sw_->allocateL3TxPacket(mtu_);
      auto buf = pkt->buf();
      int ret = 0;
      do {
        ret = read(fd_, buf->writableTail(), buf->tailroom());
      } while (ret == -1 && errno == EINTR);
      if (ret < 0) {
        if (errno != EAGAIN) {
          sysLogError(ret, "Failed to read on ", fd_);
          // Cannot continue read on this fd
          fdFail = true;
        }
        break;
      } else if (ret == 0) {
        // Nothing to read. It shall not happen as the fd is non-blocking.
        // Just add this case to be safe. Adding DCHECK for sanity checking
        // in debug mode.
        DCHECK(false) << "Unexpected event. Nothing to read.";
        break;
      } else if (ret > buf->tailroom()) {
        // The pkt is larger than the buffer. We don't have complete packet.
        // It shall not happen unless the MTU is mis-match. Drop the packet.
        LOG(ERROR) << "Too large packet (" << ret << " > " << buf->tailroom()
                   << ") received from host. Drop the packet.";
        ++dropped;
      } else {
        bytes += ret;
        buf->append(ret);
        sw_->sendL3Packet(RouterID(0), std::move(pkt));
        ++sent;
      }
    } // while
  } catch (const std::exception& ex) {
    LOG(ERROR) << "Hit some error when forwarding packets :"
               << folly::exceptionStr(ex);
  }

  if (fdFail) {
    unregisterHandler();
  }

  VLOG(4) << "Forwarded " << sent << " packets (" << bytes
          << " bytes) from host @ fd " << fd_ << " for interface " << name_
          << " dropped:" << dropped;
}

bool TunIntf::sendPacketToHost(std::unique_ptr<RxPacket> pkt) {
  CHECK(fd_ != -1);
  const int l2Len = EthHdr::SIZE;

  auto buf = pkt->buf();
  if (buf->length() <= l2Len) {
    LOG(ERROR) << "Received a too small packet with length " << buf->length();
    return false;
  }

  // skip L2 header
  buf->trimStart(l2Len);

  int ret = 0;
  do {
    ret = write(fd_, buf->data(), buf->length());
  } while (ret == -1 && errno == EINTR);
  if (ret < 0) {
    sysLogError(ret, "Failed to send packet to host from Interface ", ifID_);
    return false;
  } else if (ret < buf->length()) {
    LOG(ERROR) << "Failed to send full packet to host from Interface " << ifID_
               << ". " << ret << " bytes sent instead of " << buf->length();
    return false;
  }

  VLOG(4) << "Send packet (" << ret << " bytes) to host from Interface "
          << ifID_;
  return true;
}

}}  // namespace facebook::fboss
