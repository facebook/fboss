#include "fboss/pcap_distribution_service/PcapDistributor.h"
#include "fboss/pcap_distribution_service/ThriftHandler.h"
#include "fboss/pcap_distribution_service/PcapBufferManager.h"

#include <memory>

#include "folly/init/Init.h"

#include <folly/SocketAddress.h>
#include <folly/io/async/EventBase.h>

#include <thrift/lib/cpp/async/TAsyncSocket.h>
#include <thrift/lib/cpp2/async/HeaderClientChannel.h>
#include <thrift/lib/cpp2/server/ThriftServer.h>

#include "fboss/agent/if/gen-cpp2/FbossCtrl.h"
#include "fboss/agent/if/gen-cpp2/ctrl_constants.h"

using namespace folly;
using namespace std;

using namespace facebook::fboss;
using namespace apache::thrift;
using namespace apache::thrift::async;

int main(int argc, char** argv) {
  folly::init(&argc, &argv, true);

  auto evb = make_shared<EventBase>();

  auto dist = make_unique<PcapDistributor>(evb);
  auto buff = make_unique<PcapBufferManager>();
  auto pushsub = make_shared<ThriftHandler>(move(dist), move(buff));

  SocketAddress ctrl_address("::1", ctrl_constants::DEFAULT_CTRL_PORT());
  auto socket = TAsyncSocket::newSocket(evb.get(), ctrl_address);
  auto chan = HeaderClientChannel::newChannel(socket);
  auto client = make_unique<FbossCtrlAsyncClient>(move(chan));
  client->beginPacketDump(
      [](ClientReceiveState&& /* unused */) {},
      pcap_pubsub_constants::PCAP_PUBSUB_PORT());

  ThriftServer server;
  server.getEventBaseManager()->setEventBase(evb.get(), false);
  server.setInterface(pushsub);

  SocketAddress address;
  address.setFromLocalPort(
      pcap_pubsub_constants::PCAP_PUBSUB_PORT());
  server.setAddress(address);
  server.setup();
  LOG(INFO) << "SETUP SERVER\n";
  SCOPE_EXIT {
    server.cleanUp();
  };
  evb->loopForever();
}
