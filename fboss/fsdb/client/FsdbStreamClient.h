// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include <folly/SocketAddress.h>
#include <folly/Synchronized.h>
#include <folly/experimental/coro/BlockingWait.h>
#include <folly/io/async/ScopedEventBaseThread.h>

#include <functional>

namespace folly {
class CancellationToken;
}

namespace facebook::fboss::fsdb {
class FsdbServiceAsyncClient;

class FsdbStreamClient : public folly::AsyncTimeout {
 public:
  enum class State : uint16_t {
    DISCONNECTED,
    CONNECTING,
    CONNECTED,
    CANCELLED
  };

  using FsdbStreamStateChangeCb = std::function<void(State, State)>;
  FsdbStreamClient(
      const std::string& clientId,
      folly::EventBase* streamEvb,
      folly::EventBase* connRetryEvb,
      FsdbStreamStateChangeCb stateChangeCb = [](State /*old*/,
                                                 State /*newState*/) {});

  void setServerToConnect(
      const std::string& ip,
      uint16_t port,
      /* allow reset for use in tests*/
      bool allowReset = false);
  virtual ~FsdbStreamClient() override;
  bool isConnectedToServer() const;
  void cancel();

  bool isCancelled() const;
  const std::string& clientId() const {
    return clientId_;
  }
  State getState() const {
    return state_.load();
  }

 private:
  void createClient(const std::string& ip, uint16_t port);
  void resetClient();
  void connectToServer(const std::string& ip, uint16_t port);

  void timeoutExpired() noexcept override;

#if FOLLY_HAS_COROUTINES
  virtual folly::coro::Task<void> serviceLoop() = 0;
#endif

 protected:
  void setState(State state);
  folly::Synchronized<std::unique_ptr<folly::CancellationSource>> cancelSource_;
#ifndef IS_OSS
  std::unique_ptr<FsdbServiceAsyncClient> client_;
#endif

 private:
  std::string clientId_;
  folly::EventBase* streamEvb_;
  folly::EventBase* connRetryEvb_;
  std::atomic<State> state_{State::DISCONNECTED};
  std::unique_ptr<folly::ScopedEventBaseThread> clientEvbThread_;
  std::optional<folly::SocketAddress> serverAddress_;
  FsdbStreamStateChangeCb stateChangeCb_;
};

} // namespace facebook::fboss::fsdb
