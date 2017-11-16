#include <folly/init/Init.h>
#include "NetlinkManager.h"
#include "folly/io/async/EventBase.h"

int main(int argc, char** argv) {
  folly::init(&argc, &argv);
  folly::EventBase eb;
  facebook::fboss::NetlinkManager(&eb).run();
  eb.loopForever();
  return 0;
}
