// Copyright 2004-present Facebook. All Rights Reserved.

#pragma once

#include <common/fb303/cpp/FacebookBase2.h>
#include <fboss/agent/if/gen-cpp2/PacketStream.tcc>
namespace facebook {
namespace fboss {
class PacketStreamService : virtual public PacketStreamSvIf,
                            public facebook::fb303::FacebookBase2 {
 public:
  explicit PacketStreamService(const std::string& serviceName)
      : facebook::fb303::FacebookBase2(serviceName.c_str()) {}
  virtual ~PacketStreamService() override;

  // helper functions.
  void send(const std::string& clientId, TPacket&& packet);
  bool isClientConnected(const std::string& clientId);
  bool isPortRegistered(const std::string& clientId, const std::string& port);

  // thrift overrides.
  facebook::fb303::cpp2::fb_status getStatus() override {
    return facebook::fb303::cpp2::fb_status::ALIVE;
  }
  apache::thrift::ServerStream<TPacket> connect(
      std::unique_ptr<std::string> clientId) override;
  void registerPort(
      std::unique_ptr<std::string> clientId,
      std::unique_ptr<std::string> l2Port) override;
  void clearPort(
      std::unique_ptr<std::string> clientId,
      std::unique_ptr<std::string> l2Port) override;
  void disconnect(std::unique_ptr<std::string> clientId) override;

 protected:
  // Service extending the PacketStreamService should implement the
  // following methods.
  virtual void clientConnected(const std::string& clientId) = 0;
  virtual void clientDisconnected(const std::string& clientId) = 0;
  virtual void addPort(
      const std::string& clientId,
      const std::string& l2Port) = 0;
  virtual void removePort(
      const std::string& clientId,
      const std::string& l2Port) = 0;

 private:
  struct ClientInfo {
    explicit ClientInfo(apache::thrift::ServerStreamPublisher<TPacket> pub)
        : publisher_(
              std::make_unique<apache::thrift::ServerStreamPublisher<TPacket>>(
                  std::move(pub))) {}
    std::unordered_set<std::string> portList_;
    std::unique_ptr<apache::thrift::ServerStreamPublisher<TPacket>> publisher_;
  };
  using ClientMap = std::unordered_map<std::string, ClientInfo>;
  folly::Synchronized<ClientMap> clientMap_;
};

} // namespace fboss
} // namespace facebook
