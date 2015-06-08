#include "fboss/agent/if/gen-cpp2/FbossCtrl.h"
#include "fboss/agent/if/gen-cpp2/PortStatusListenerClient.h"

#include <iostream>
#include <string>
#include <chrono>
#include <gflags/gflags.h>
#include <thrift/lib/cpp2/async/DuplexChannel.h>
#include <thrift/lib/cpp/transport/TSocketAddress.h>
#include <thrift/lib/cpp2/server/ThriftServer.h>
#include <thrift/lib/cpp/util/ScopedServerThread.h>
#include <thrift/lib/cpp/async/TEventBase.h>
#include <thrift/lib/cpp/async/TAsyncSocket.h>

using namespace apache::thrift;
using namespace apache::thrift::util;
using namespace apache::thrift::async;
using namespace apache::thrift::transport;
using namespace facebook::fboss;
using namespace folly;

DEFINE_string(host, "", "The host to connect to");
DEFINE_int32(port, 5909, "The port to connect to");

class PortStatusListenerClientInterface : public PortStatusListenerClientSvIf {
 public:
  void async_tm_portStatusChanged(
      std::unique_ptr<apache::thrift::HandlerCallback<void>> cb,
      int32_t id,
      std::unique_ptr<facebook::fboss::PortStatus> ps) override {
     std::cout << "Port status changed: " << id << "\n";
   }
};

int main(int argc, char **argv) {
  TEventBase base;

  google::ParseCommandLineFlags(&argc, &argv, true);
  google::InitGoogleLogging(argv[0]);
  google::InstallFailureSignalHandler();

  TSocketAddress addr(FLAGS_host, FLAGS_port, true);
  auto socket = TAsyncSocket::newSocket(&base, addr);
  auto chan =
      std::make_shared<DuplexChannel>(DuplexChannel::Who::CLIENT, socket);
  ThriftServer clients_server(chan->getServerChannel());
  clients_server.setIdleTimeout(std::chrono::milliseconds(0));
  clients_server.setInterface(
      std::make_shared<PortStatusListenerClientInterface>());
  clients_server.serve();

  FbossCtrlAsyncClient client(chan->getClientChannel());
  client.registerForPortStatusChanged([](ClientReceiveState&& state) {
    PortStatus ps;
    FbossCtrlAsyncClient::recv_registerForPortStatusChanged(state);
    LOG(INFO) << "registered for port status on " << FLAGS_host << "\n";
  });

  base.loopForever();
}
