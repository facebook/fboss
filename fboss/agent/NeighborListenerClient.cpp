#include "fboss/agent/if/gen-cpp2/FbossCtrl.h"
#include "fboss/agent/if/gen-cpp2/NeighborListenerClient.h"

#include <iostream>
#include <string>
#include <chrono>
#include <gflags/gflags.h>
#include <folly/SocketAddress.h>
#include <folly/io/async/EventBase.h>
#include <thrift/lib/cpp2/async/DuplexChannel.h>
#include <thrift/lib/cpp2/server/ThriftServer.h>
#include <thrift/lib/cpp/util/ScopedServerThread.h>
#include <thrift/lib/cpp/async/TAsyncSocket.h>

using namespace apache::thrift;
using namespace apache::thrift::util;
using namespace apache::thrift::async;
using namespace apache::thrift::transport;
using namespace facebook::fboss;
using namespace folly;

DEFINE_string(host, "", "The host to connect to");
DEFINE_int32(port, 5909, "The port to connect to");

class NeighborListenerClientInterface : public NeighborListenerClientSvIf {
 public:
  void async_tm_neighboursChanged(
      std::unique_ptr<apache::thrift::HandlerCallback<void>> cb,
      std::unique_ptr<std::vector<std::string>> added,
      std::unique_ptr<std::vector<std::string>> removed) {
    for (const auto& up : *added) {
      LOG(INFO) << "neighbour added: " << up << "\n";
    }
    for (const auto& down : *removed) {
      LOG(INFO) << "neighbour added: " << down << "\n";
    }
  }
};

int main(int argc, char **argv) {
  folly::EventBase base;

  google::ParseCommandLineFlags(&argc, &argv, true);
  google::InitGoogleLogging(argv[0]);
  google::InstallFailureSignalHandler();

  SocketAddress addr(FLAGS_host, FLAGS_port, true);
  auto socket = TAsyncSocket::newSocket(&base, addr);
  auto chan =
      std::make_shared<DuplexChannel>(DuplexChannel::Who::CLIENT, socket);
  ThriftServer clients_server(chan->getServerChannel());
  clients_server.setIdleTimeout(std::chrono::milliseconds(0));
  clients_server.setInterface(
      std::make_shared<NeighborListenerClientInterface>());
  clients_server.serve();

  FbossCtrlAsyncClient client(chan->getClientChannel());
  client.registerForNeighborChanged([](ClientReceiveState&& state) {
    PortStatus ps;
    FbossCtrlAsyncClient::recv_registerForNeighborChanged(state);
    LOG(INFO) << "registered for port status on " << FLAGS_host << "\n";
  });

  base.loopForever();
}
