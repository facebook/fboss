// (c) Facebook, Inc. and its affiliates. Confidential and proprietary.

#include <folly/ThreadLocal.h>
#include "fboss/qsfp_service/if/gen-cpp2/QsfpService.h"
#include <folly/init/Init.h>
#include <folly/experimental/coro/BlockingWait.h>
#include <folly/io/async/AsyncSocket.h>
#include <thrift/lib/cpp2/async/HeaderClientChannel.h>
#include <folly/json.h>
#include <folly/FileUtil.h>
#include <folly/Random.h>
#include <folly/io/async/ScopedEventBaseThread.h>
#include "fboss/qsfp_service/lib/QsfpClient.h"

using namespace facebook::fboss;
using namespace facebook::fboss::mka;
using namespace apache::thrift;

int main(int argc, char* argv[]) {
  folly::init(&argc, &argv, true);
  gflags::SetCommandLineOptionWithMode(
      "minloglevel", "0", gflags::SET_FLAGS_DEFAULT);
  folly::EventBase evb;

  std::unique_ptr<QsfpServiceAsyncClient> client;
  client = QsfpClient::createClient(&evb).getVia(&evb);

  return 0;
}
