namespace py3 neteng.fboss
namespace py neteng.fboss.fsdb
namespace py.asyncio neteng.fboss.asyncio.fsdb
namespace cpp2 facebook.fboss.fsdb
namespace go facebook.fboss.fsdb

include "common/fb303/if/fb303.thrift"
include "fboss/fsdb/if/fsdb_common.thrift"
include "fboss/fsdb/if/fsdb_oper.thrift"
include "thrift/annotation/cpp.thrift"

cpp_include "folly/container/F14Map.h"
cpp_include "folly/FBString.h"

struct OperPublisherInfo {
  1: fsdb_common.PublisherId publisherId;
  2: fsdb_oper.PubSubType type;
  3: fsdb_oper.OperPath path;
  4: bool isStats;
  5: bool isExpectedPath;
}

// could probably reuse types between get/subscribe here, but easier
// to have different types if the apis end up diverging.
struct OperGetRequest {
  1: fsdb_oper.OperPath path;
  2: fsdb_oper.OperProtocol protocol = fsdb_oper.OperProtocol.BINARY;
}

struct OperGetRequestExtended {
  1: list<fsdb_oper.ExtendedOperPath> paths;
  2: fsdb_oper.OperProtocol protocol = fsdb_oper.OperProtocol.BINARY;
}

struct OperSubscriberInfo {
  1: fsdb_common.SubscriberId subscriberId;
  2: fsdb_oper.PubSubType type;
  3: optional fsdb_oper.OperPath path;
  4: bool isStats;
  5: optional list<fsdb_oper.ExtendedOperPath> extendedPaths;
  6: optional i64 subscribedSince;
  // Paths for Patch apis
  // TODO: replace path above
  7: optional map<fsdb_oper.SubscriptionKey, fsdb_oper.RawOperPath> paths;
  8: optional i64 subscriptionUid;
  9: optional i32 subscriptionQueueWatermark;
}

@cpp.Type{template = "folly::F14FastMap"}
typedef map<
  fsdb_common.PublisherId,
  list<OperPublisherInfo>
> PublisherIdToOperPublisherInfo
@cpp.Type{template = "folly::F14FastMap"}
typedef map<
  fsdb_common.SubscriberId,
  list<OperSubscriberInfo>
> SubscriberIdToOperSubscriberInfos

service FsdbService extends fb303.FacebookService {
  // OperState APIs

  fsdb_oper.OperPubInitResponse, sink<
    fsdb_oper.OperState throws (1: fsdb_common.FsdbException e),
    fsdb_oper.OperPubFinalResponse throws (1: fsdb_common.FsdbException e)
  > publishOperStatePath(1: fsdb_oper.OperPubRequest request) throws (
    1: fsdb_common.FsdbException e,
  );

  fsdb_oper.OperSubInitResponse, stream<
    fsdb_oper.OperState throws (1: fsdb_common.FsdbException e)
  > subscribeOperStatePath(1: fsdb_oper.OperSubRequest request) throws (
    1: fsdb_common.FsdbException e,
  );

  fsdb_oper.OperPubInitResponse, sink<
    fsdb_oper.OperDelta throws (1: fsdb_common.FsdbException e),
    fsdb_oper.OperPubFinalResponse throws (1: fsdb_common.FsdbException e)
  > publishOperStateDelta(1: fsdb_oper.OperPubRequest request) throws (
    1: fsdb_common.FsdbException e,
  );

  fsdb_oper.OperSubInitResponse, stream<
    fsdb_oper.OperDelta throws (1: fsdb_common.FsdbException e)
  > subscribeOperStateDelta(1: fsdb_oper.OperSubRequest request) throws (
    1: fsdb_common.FsdbException e,
  );

  fsdb_oper.OperSubInitResponse, stream<
    fsdb_oper.OperSubPathUnit throws (1: fsdb_common.FsdbException e)
  > subscribeOperStatePathExtended(
    1: fsdb_oper.OperSubRequestExtended request,
  ) throws (1: fsdb_common.FsdbException e);

  fsdb_oper.OperSubInitResponse, stream<
    fsdb_oper.OperSubDeltaUnit throws (1: fsdb_common.FsdbException e)
  > subscribeOperStateDeltaExtended(
    1: fsdb_oper.OperSubRequestExtended request,
  ) throws (1: fsdb_common.FsdbException e);

  // OperStats APIs

  fsdb_oper.OperPubInitResponse, sink<
    fsdb_oper.OperState throws (1: fsdb_common.FsdbException e),
    fsdb_oper.OperPubFinalResponse throws (1: fsdb_common.FsdbException e)
  > publishOperStatsPath(1: fsdb_oper.OperPubRequest request) throws (
    1: fsdb_common.FsdbException e,
  );

  fsdb_oper.OperSubInitResponse, stream<
    fsdb_oper.OperState throws (1: fsdb_common.FsdbException e)
  > subscribeOperStatsPath(1: fsdb_oper.OperSubRequest request) throws (
    1: fsdb_common.FsdbException e,
  );

  fsdb_oper.OperPubInitResponse, sink<
    fsdb_oper.OperDelta throws (1: fsdb_common.FsdbException e),
    fsdb_oper.OperPubFinalResponse throws (1: fsdb_common.FsdbException e)
  > publishOperStatsDelta(1: fsdb_oper.OperPubRequest request) throws (
    1: fsdb_common.FsdbException e,
  );

  fsdb_oper.OperSubInitResponse, stream<
    fsdb_oper.OperDelta throws (1: fsdb_common.FsdbException e)
  > subscribeOperStatsDelta(1: fsdb_oper.OperSubRequest request) throws (
    1: fsdb_common.FsdbException e,
  );

  fsdb_oper.OperSubInitResponse, stream<
    fsdb_oper.OperSubPathUnit throws (1: fsdb_common.FsdbException e)
  > subscribeOperStatsPathExtended(
    1: fsdb_oper.OperSubRequestExtended request,
  ) throws (1: fsdb_common.FsdbException e);

  fsdb_oper.OperSubInitResponse, stream<
    fsdb_oper.OperSubDeltaUnit throws (1: fsdb_common.FsdbException e)
  > subscribeOperStatsDeltaExtended(
    1: fsdb_oper.OperSubRequestExtended request,
  ) throws (1: fsdb_common.FsdbException e);

  fsdb_oper.OperState getOperState(1: OperGetRequest request) throws (
    1: fsdb_common.FsdbException e,
  );
  fsdb_oper.OperState getOperStats(1: OperGetRequest request) throws (
    1: fsdb_common.FsdbException e,
  );

  list<fsdb_oper.TaggedOperState> getOperStateExtended(
    1: OperGetRequestExtended request,
  ) throws (1: fsdb_common.FsdbException e);
  list<fsdb_oper.TaggedOperState> getOperStatsExtended(
    1: OperGetRequestExtended request,
  ) throws (1: fsdb_common.FsdbException e);

  // New Oper APIs
  // TODO: phase out above apis
  fsdb_oper.OperPubInitResponse, sink<
    fsdb_oper.PublisherMessage throws (1: fsdb_common.FsdbException e),
    fsdb_oper.OperPubFinalResponse throws (1: fsdb_common.FsdbException e)
  > publishState(1: fsdb_oper.PubRequest request) throws (
    1: fsdb_common.FsdbException e,
  );

  fsdb_oper.OperPubInitResponse, sink<
    fsdb_oper.PublisherMessage throws (1: fsdb_common.FsdbException e),
    fsdb_oper.OperPubFinalResponse throws (1: fsdb_common.FsdbException e)
  > publishStats(1: fsdb_oper.PubRequest request) throws (
    1: fsdb_common.FsdbException e,
  );

  fsdb_oper.OperSubInitResponse, stream<
    fsdb_oper.SubscriberMessage throws (1: fsdb_common.FsdbException e)
  > subscribeState(1: fsdb_oper.SubRequest request) throws (
    1: fsdb_common.FsdbException e,
  );

  fsdb_oper.OperSubInitResponse, stream<
    fsdb_oper.SubscriberMessage throws (1: fsdb_common.FsdbException e)
  > subscribeStats(1: fsdb_oper.SubRequest request) throws (
    1: fsdb_common.FsdbException e,
  );

  // Custom Oper getters: add specific state getters here if
  // desired.

  // FSDB Management Plane related ------------------------------------------

  // Oper related

  PublisherIdToOperPublisherInfo getAllOperPublisherInfos() throws (
    1: fsdb_common.FsdbException e,
  );
  PublisherIdToOperPublisherInfo getOperPublisherInfos(
    1: fsdb_common.PublisherIds publisherIds,
  ) throws (1: fsdb_common.FsdbException e);

  SubscriberIdToOperSubscriberInfos getAllOperSubscriberInfos() throws (
    1: fsdb_common.FsdbException e,
  );
  SubscriberIdToOperSubscriberInfos getOperSubscriberInfos(
    1: fsdb_common.SubscriberIds subscriberIds,
  ) throws (1: fsdb_common.FsdbException e);
}
