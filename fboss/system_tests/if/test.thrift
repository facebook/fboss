include "common/fb303/if/fb303.thrift"
include "common/network/if/Address.thrift"

namespace py fboss.system_tests.test

service TestService extends fb303.FacebookService {
  bool ping(Address.BinaryAddress ip);
  bool status();
}
