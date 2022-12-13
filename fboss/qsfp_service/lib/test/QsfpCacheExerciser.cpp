#include <chrono>
#include <iostream>

#include "fboss/agent/if/gen-cpp2/ctrl_clients.h"
#include "fboss/agent/if/gen-cpp2/ctrl_types.h"
#include "fboss/agent/types.h"
#include "fboss/qsfp_service/lib/QsfpCache.h"

#include <folly/Random.h>
#include <folly/experimental/FunctionScheduler.h>
#include <folly/init/Init.h>
#include <folly/io/async/EventBase.h>

#include <folly/io/async/AsyncSocket.h>
#include <folly/logging/xlog.h>
#include <thrift/lib/cpp2/async/HeaderClientChannel.h>

DECLARE_string(qsfp_service_host);
DEFINE_bool(dump_qsfp_cache, false, "dump contents of cache as we go");
DEFINE_uint32(port_change_interval, 3, "How often (in secs) to change ports");
DEFINE_uint32(start_delay, 5, "start delay before generating syncPorts calls");

namespace facebook {
namespace fboss {

namespace {
// TODO(aeckert): library with various fboss cpp thrift clients would
// be really useful
using namespace apache::thrift;

std::unique_ptr<apache::thrift::Client<FbossCtrl>> fbossAgentClient() {
  folly::EventBase* eb = folly::EventBaseManager::get()->getEventBase();
  folly::SocketAddress agent(FLAGS_qsfp_service_host, 5909);
  auto socket = folly::AsyncSocket::newSocket(eb, agent);
  auto chan = HeaderClientChannel::newChannel(std::move(socket));
  return std::make_unique<apache::thrift::Client<FbossCtrl>>(std::move(chan));
}

} // namespace
} // namespace fboss
} // namespace facebook

int main(int argc, char** argv) {
  folly::init(&argc, &argv);

  folly::EventBase evb;
  facebook::fboss::QsfpCache qc;

  auto ac = facebook::fboss::fbossAgentClient();

  std::map<int32_t, facebook::fboss::PortStatus> ret;
  std::vector<int32_t> ports;
  ac->sync_getPortStatus(ret, ports);

  qc.init(&evb, ret);
  auto size = ret.size();
  std::cout << "initialized " << size;

  auto poke = [&]() {
    for (auto& item : ret) {
      if ((folly::Random::rand32() % 10) == 0) {
        XLOG(INFO) << "calling portChanged w/ " << item.first;
        qc.portChanged(item.first, item.second);
      }
    }

    // Query the cache a bit
    std::vector<folly::Future<facebook::fboss::TransceiverInfo>> futs;
    for (int i = 0; i < 40; ++i) {
      if ((folly::Random::rand32() % 10) == 0) {
        XLOG(INFO) << "querying for transceiver " << i;
        auto fut = qc.futureGet(facebook::fboss::TransceiverID(i))
                       .thenValue([i](auto&& info) {
                         XLOG(INFO) << "Successfully got transceiver " << i;
                         return std::move(info);
                       })
                       .thenError(
                           folly::tag_t<std::runtime_error>{},
                           [i](std::runtime_error exc) {
                             XLOG(ERR) << "Error retrieving info for tcvr " << i
                                       << ": " << exc.what();
                             return facebook::fboss::TransceiverInfo();
                           });
        futs.push_back(std::move(fut));
      }
    }
    if (futs.size() > 0) {
      folly::collectAll(futs).toUnsafeFuture().thenValue([](auto&&) {
        XLOG(INFO) << "Retrieved all desired transceivers from cache";
      });
    }

    if (FLAGS_dump_qsfp_cache) {
      qc.dump();
    }
  };

  folly::FunctionScheduler scheduler;
  scheduler.addFunction(
      poke,
      std::chrono::seconds(FLAGS_port_change_interval),
      "syncRandom",
      std::chrono::seconds(FLAGS_start_delay));
  scheduler.start();

  evb.loopForever();
}
