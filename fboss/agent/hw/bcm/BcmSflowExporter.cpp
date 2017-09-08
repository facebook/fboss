/*
 *  Copyright (c) 2004-present, Facebook, Inc.
 *  All rights reserved.
 *
 *  This source code is licensed under the BSD-style license found in the
 *  LICENSE file in the root directory of this source tree. An additional grant
 *  of patent rights can be found in the PATENTS file in the same directory.
 *
 */
#include "BcmSflowExporter.h"

#include <fcntl.h>

#include <glog/logging.h>

#include <thrift/lib/cpp2/protocol/Serializer.h>

#include "fboss/agent/FbossError.h"


using namespace std;

namespace facebook { namespace fboss {

BcmSflowExporter::BcmSflowExporter(const folly::SocketAddress& address)
    : address_(address) {
  SCOPE_FAIL {
    close(socket_);
  };

  socket_ = ::socket(address_.getFamily(), SOCK_DGRAM, IPPROTO_UDP);

  if (socket_ == -1) {
    throw FbossError("Error creating UDP socket: ", folly::errnoStr(errno));
  }

  // put the socket in non-blocking mode
  if (fcntl(socket_, F_SETFL, O_NONBLOCK) != 0) {
    throw FbossError(
        "Failed to put socket in non-blocking mode: ", folly::errnoStr(errno));
  }

  // put the socket in reuse mode
  int val = 1;
  if (setsockopt(socket_, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(val)) != 0) {
    throw FbossError(
        "Failed to put socket in reuse_addr mode: ", folly::errnoStr(errno));
  }

  // put the socket in port reuse mode
  val = 1;
  if (setsockopt(socket_, SOL_SOCKET, SO_REUSEPORT, &val, sizeof(val)) != 0) {
    throw FbossError(
        "Failed to put socket in reuse_port mode: ", folly::errnoStr(errno));
  }

  // bind the socket.
  folly::SocketAddress localAddr("::", 0);
  sockaddr_storage addrStorage;
  localAddr.getAddress(&addrStorage);
  sockaddr* saddr = reinterpret_cast<sockaddr*>(&addrStorage);
  if (bind(socket_, saddr, localAddr.getActualSize()) != 0) {
    throw FbossError(
        "Failed to bind the async udp socket for ",
        address_.describe(),
        ": ",
        folly::errnoStr(errno));
  }
}

ssize_t BcmSflowExporter::sendUDPDatagram(iovec* vec, const size_t iovec_len) {
  VLOG(4) << "Sending an sFlow packet to " << address_.describe();

  sockaddr_storage addrStorage;
  address_.getAddress(&addrStorage);

  struct msghdr msg = {};
  msg.msg_name = reinterpret_cast<void*>(&addrStorage);
  msg.msg_namelen = address_.getActualSize();
  msg.msg_iov = vec;
  msg.msg_iovlen = iovec_len;
  msg.msg_control = nullptr;
  msg.msg_controllen = 0;
  msg.msg_flags = 0;
  auto ret = ::sendmsg(socket_, &msg, 0);
  if (ret < 0) {
    VLOG(1) << "Failed sending sFlow packet to " << address_.describe()
            << " reason: " << folly::errnoStr(errno);
  }
  VLOG(4) << "Sent " << ret << " bytes of sFlow packet to "
          << address_.describe();
  return ret;
}

BcmSflowExporter::~BcmSflowExporter() {
  if (socket_ != -1) {
    close(socket_);
  }
}

bool BcmSflowExporterTable::contains(
    const shared_ptr<SflowCollector>& c) const {
  auto iter = map_.find(c->getID());
  return iter != map_.end();
}

size_t BcmSflowExporterTable::size() const {
  return map_.size();
}

void BcmSflowExporterTable::addExporter(const shared_ptr<SflowCollector>& c) {
  try {
    auto exporter = make_unique<BcmSflowExporter>(c->getAddress());
    map_.emplace(c->getID(), move(exporter));
  } catch (const fboss::thrift::FbossBaseError& ex) {
    LOG(ERROR) << "Could not add exporter: "
               << c->getAddress().getFullyQualified()
               << " reason: " << folly::exceptionStr(ex);
  }

  LOG(INFO) << "Successfully added exporter for "
            << c->getAddress().getFullyQualified();
}

void BcmSflowExporterTable::removeExporter(const std::string& id) {
  map_.erase(id);
}

void BcmSflowExporterTable::sendToAll(const SflowPacketInfo& info) {
  if (map_.empty()) {
    VLOG(1)
        << "zero sFlow collectors with sflow enabled, skipping sample export";
    return;
  }
  // Serialize info to a string and wrap it in an IOBuf for sending
  string output;
  apache::thrift::BinarySerializer::serialize(info, &output);
  auto buf = folly::IOBuf::wrapBuffer(output.data(), output.length());

  // Map the IOBuf to a Linux iovec like a champ
  iovec vec[16] = {};
  size_t iovec_len = buf->fillIov(vec, sizeof(vec) / sizeof(vec[0]));
  if (UNLIKELY(iovec_len == 0)) {
    buf->coalesce();
    vec[0].iov_base = const_cast<uint8_t*>(buf->data());
    vec[0].iov_len = buf->length();
    iovec_len = 1;
  }

  for (const auto& c: map_) {
    // TODO: prob need to handle ret code?
    c.second->sendUDPDatagram(vec, iovec_len);
  }
}

}}
