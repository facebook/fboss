namespace cpp2 facebook.fboss.state
namespace go neteng.fboss.qsfp_state
namespace py neteng.fboss.qsfp_state
namespace py3 neteng.fboss
namespace py.asyncio neteng.fboss.asyncio.qsfp_state

include "fboss/qsfp_service/if/qsfp_service_config.thrift"

struct QsfpState {}

struct QsfpServiceData {
  1: qsfp_service_config.QsfpServiceConfig config;
  2: QsfpState state;
}
