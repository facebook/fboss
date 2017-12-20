#include <folly/init/Init.h>
#include "NetlinkManager.h"
#include "folly/io/async/EventBase.h"

int main(int argc, char** argv) {
  folly::init(&argc, &argv);
  auto netlinkManagerThread = std::make_unique<std::thread>([] {
    folly::EventBase eb;
    auto nlm = std::make_unique<facebook::fboss::NetlinkManager>(&eb);
    nlm->run();
    eb.loopForever();
  });
  netlinkManagerThread->join();
  return 0;
}
