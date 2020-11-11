// Copyright 2004-present Facebook.  All rights reserved.
#pragma once

#include <folly/io/IOBuf.h>
#include <folly/io/async/AsyncSocketBase.h>
#include <folly/io/async/AsyncSocketException.h>

namespace facebook {
namespace fboss {

// AsycnPacketTransport will follow AsyncSocket Paradigm as much as possible
// to make it easier to follow

class AsyncPacketTransport {
 public:
  class ReadCallback {
   public:
    /**
     * Invoked when a new packet is available on the socket.
     */
    virtual void onDataAvailable(
        std::unique_ptr<folly::IOBuf> buf) noexcept = 0;

    /**
     * Invoked when there is an error reading from the socket.
     */
    virtual void onReadError(
        const folly::AsyncSocketException& ex) noexcept = 0;

    /**
     * Invoked when socket is closed and a read callback is registered.
     */
    virtual void onReadClosed() noexcept = 0;

    virtual ~ReadCallback() = default;
  };

  explicit AsyncPacketTransport(std::string iface) : iface_(std::move(iface)) {}

  virtual ~AsyncPacketTransport() = default;

  /**
   * Returns the network interface it's bound to
   */
  virtual const std::string& iface() const {
    return iface_;
  }
  /**
   * Send the data in buffer to destination. Returns the return code from
   * ::send.
   */
  virtual ssize_t send(const std::unique_ptr<folly::IOBuf>& buf) = 0;

  /**
   * Stop listening on the socket.
   */
  virtual void close() = 0;

  virtual void setReadCallback(ReadCallback* readCallback) {
    readCallback_ = readCallback;
  }

  virtual bool isReading() const {
    return readCallback_ != nullptr;
  }

 protected:
  std::string iface_;
  ReadCallback* readCallback_{nullptr};

 private:
  AsyncPacketTransport(const AsyncPacketTransport&) = delete;
  AsyncPacketTransport& operator=(const AsyncPacketTransport&) = delete;
};

} // namespace fboss
} // namespace facebook
