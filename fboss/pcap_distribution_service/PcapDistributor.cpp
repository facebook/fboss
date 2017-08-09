#include "fboss/pcap_distribution_service/PcapDistributor.h"

#include <functional>
#include <memory>
#include <map>
#include <tuple>
#include <folly/Logging.h>

#include <folly/SocketAddress.h>
#include <folly/Synchronized.h>
#include <folly/io/async/EventBase.h>

#include <thrift/lib/cpp/async/TAsyncSocket.h>
#include <thrift/lib/cpp/server/TServer.h>
#include <thrift/lib/cpp2/async/HeaderClientChannel.h>
#include <thrift/lib/cpp2/server/ThriftServer.h>

#include "fboss/pcap_distribution_service/if/gen-cpp2/PcapPushSubscriber.h"
#include "fboss/pcap_distribution_service/if/gen-cpp2/PcapSubscriber.h"

using namespace std;
using namespace folly;
using namespace apache::thrift::server;
using namespace apache::thrift::async;
using namespace apache::thrift;

namespace facebook {
namespace fboss {

void PcapDistributor::subscribe(unique_ptr<string> hostname, int port) {
  auto creation = [&, hostname = move(hostname), port ]() {
    auto locked_map = subs_.wlock();
    auto locked_callbacks = callbacks_.wlock();
    SocketAddress addr(*hostname, port, true);
    auto socket = TAsyncSocket::newSocket(evb_.get(), addr);
    auto chan = HeaderClientChannel::newChannel(socket);
    auto key = pair<string, int>(*hostname, port);
    ChannelCloserCB closer(this, key);
    locked_map->erase(key);
    locked_callbacks->erase(key);
    locked_callbacks->emplace(key, move(closer));
    chan->setCloseCallback(&locked_callbacks->at(key));
    locked_map->emplace(
        key, make_unique<PcapSubscriberAsyncClient>(move(chan)));
    LOG(INFO) << "CREATED SUBSCRIBER: " << *hostname << " " << port;
  };
  evb_->runInEventBaseThread(move(creation));
}

void PcapDistributor::unsubscribe(const string& hostname, int port) {
  subs_.wlock()->erase(pair<string, int>(hostname, port));
  callbacks_.wlock()->erase(pair<string, int>(hostname, port));
  LOG(INFO) << "UNSUBSCRIBED CLIENT: " << hostname << " " << port;
}

void PcapDistributor::distributeRxPacket(unique_ptr<RxPacketData> packetData) {
  auto locked_map = subs_.rlock();
  auto onError = [](runtime_error& e) {
    FB_LOG_EVERY_MS(ERROR, 1000) << e.what();
  };
  for (auto& i : *locked_map) {
    i.second->future_receiveRxPacket(*packetData).onError(move(onError));
  }
}

void PcapDistributor::distributeTxPacket(unique_ptr<TxPacketData> packetData) {
  auto locked_map = subs_.rlock();
  auto onError = [](runtime_error& e) {
    FB_LOG_EVERY_MS(ERROR, 1000) << e.what();
  };
  for (auto& i : *locked_map) {
    i.second->future_receiveTxPacket(*packetData).onError(move(onError));
  }
}

void PushSubscriber::subscribe(unique_ptr<string> hostname, int port) {
  dist_->subscribe(move(hostname), port);
}
void PushSubscriber::unsubscribe(unique_ptr<string> hostname, int port) {
  dist_->unsubscribe(*hostname, port);
}
void PushSubscriber::receiveRxPacket(unique_ptr<RxPacketData> packetData) {
  dist_->distributeRxPacket(move(packetData));
}
void PushSubscriber::receiveTxPacket(unique_ptr<TxPacketData> packetData) {
  dist_->distributeTxPacket(move(packetData));
}
void PushSubscriber::kill() {
  LOG(INFO) << "KILLED FROM AGENT";
  exit(0);
}
}
}
