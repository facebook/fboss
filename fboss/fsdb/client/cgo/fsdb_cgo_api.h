// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

#ifndef FBOSS_FSDB_CLIENT_CGO_FSDB_CGO_API_H
#define FBOSS_FSDB_CLIENT_CGO_FSDB_CGO_API_H

#include <stdint.h>

#ifdef __cplusplus
#define FSDB_CGO_API extern "C" __attribute__((visibility("default")))
#else
#define FSDB_CGO_API __attribute__((visibility("default")))
#endif

// Bump on any change to a struct layout, function signature, or enum value.
// Pass this value to FsdbInit() — the library compares it against its own
// baked-in version and refuses to initialize on mismatch.
#define FSDB_CGO_ABI_VERSION 2

// Returns the ABI version baked into the shipped library. Most consumers
// should not call this directly — pass FSDB_CGO_ABI_VERSION to FsdbInit() and
// let it do the comparison.
FSDB_CGO_API int32_t FsdbCgoAbiVersion(void);

// This header is also valid C — Go's CGO preamble compiles it as C, not C++.
// Keep the C-style typedefs; do NOT let clang-tidy modernize them to `using`
// (which is C++-only syntax and would break the cgo include).
// NOLINTBEGIN(modernize-use-using)
typedef void* FsdbWrapperHandle;

typedef struct {
  const char* port_name;
  int32_t port_id;
  int32_t oper_state;
} FsdbPortStateUpdate;

// data is raw Thrift-serialized OperState contents.
// protocol is the OperProtocol enum value (BINARY=1, COMPACT=2, SIMPLE_JSON=3).
typedef struct {
  const char* key;
  const uint8_t* data;
  int32_t data_len;
  int32_t protocol;
} FsdbStatsUpdate;

typedef struct {
  const char* key;
  const uint8_t* data;
  int32_t data_len;
  int32_t protocol;
} FsdbStatePathUpdate;

// Connection state of the portMaps subscription, returned by
// GetConnectionState. Values increase monotonically through a connection's
// lifecycle: CONNECTING (before the first successful connect, e.g. server
// unreachable) -> CONNECTED (stream established, no data yet) -> DATA_RECEIVED
// (initial sync / first data delivered). A live drop resets to DISCONNECTED.
typedef enum {
  FSDB_CONNECTION_DISCONNECTED = 0,
  FSDB_CONNECTION_CONNECTING = 1,
  FSDB_CONNECTION_CONNECTED = 2,
  FSDB_CONNECTION_DATA_RECEIVED = 3,
} FsdbConnectionState;
// NOLINTEND(modernize-use-using)

// Set a folly/gflags flag value. Call before FsdbInit for the value to be
// observed by Folly bootstrap; calls after FsdbInit still update gflags but
// folly singletons may already have captured the old value.
//
// Vendors should use this instead of fabricating an argv array. Common flags:
//   "logging"             — folly logging spec, e.g. "DBG2", "fboss.fsdb=DBG4"
//   "fsdb_reconnect_ms"   — FSDB stream reconnect interval (default ~1000)
//   "logtostderr"         — "1" to mirror glog output to stderr
//
// Both name and value are copied; caller can free them after the call returns.
//
// Returns:
//   0 — success: flag exists and the new value was applied
//   1 — failure: null args, unknown flag, value couldn't be parsed, or
//       internal error (logged to stderr)
FSDB_CGO_API int32_t FsdbSetFlag(const char* name, const char* value);

// One-time process-wide initialization of the C++ runtime (Folly singletons,
// gflags). Idempotent across calls. Consumers MUST pass FSDB_CGO_ABI_VERSION
// from the header they compiled against; the library compares it against its
// own baked-in version and refuses to initialize on mismatch (returns 2).
//
// Returns:
//   0 — success (or no-op when already initialized)
//   1 — caught exception during Folly init
//   2 — ABI version mismatch (caller should abort; do NOT proceed to use any
//       wrapper API, struct layouts may differ from what the caller expects)
FSDB_CGO_API int32_t FsdbInit(int32_t consumer_abi_version);

FSDB_CGO_API FsdbWrapperHandle CreateFsdbWrapper(const char* clientId);
FSDB_CGO_API void DestroyFsdbWrapper(FsdbWrapperHandle handle);

// Signal the wrapper to stop blocking inside WaitFor* calls. Returns 0 on
// success, 1 if an exception was caught. Idempotent and null-safe.
FSDB_CGO_API int32_t ShutdownFsdbWrapper(FsdbWrapperHandle handle);

// All subscribe entry points take a host + port. host == NULL or "" connects
// to localhost (::1). server_port < 0 uses the default FSDB port (in which case
// host is ignored — pass an explicit port to reach a remote host).
FSDB_CGO_API void SubscribeToPortMaps(
    FsdbWrapperHandle handle,
    const char* host,
    int32_t server_port);

FSDB_CGO_API void SubscribeToStats(
    FsdbWrapperHandle handle,
    const char** path_tokens,
    int32_t num_tokens,
    const char* host,
    int32_t server_port);

FSDB_CGO_API void SubscribeToState(
    FsdbWrapperHandle handle,
    const char** path_tokens,
    int32_t num_tokens,
    const char* host,
    int32_t server_port);

// Synchronous one-shot snapshot of all ports (no subscription required), for
// reconciliation. Fetches agent/switchState/portMaps from FSDB, decodes it, and
// fills up to max_count records into out. host/server_port follow the same rule
// as the subscribe calls (host NULL/"" => localhost; server_port < 0 => default
// port, host ignored). Returns the number of ports written (>=0), or -1 on
// error. Borrowed port_name pointers stay valid until the next GetPortSnapshot
// call on this handle or DestroyFsdbWrapper.
FSDB_CGO_API int32_t GetPortSnapshot(
    FsdbWrapperHandle handle,
    const char* host,
    int32_t server_port,
    FsdbPortStateUpdate* out,
    int32_t max_count);

// Live connection state of the portMaps subscription (a FsdbConnectionState).
// SubscribeToPortMaps returns before the connection is established, so poll
// this to tell "still connecting" from "stream up but no data yet" from "data
// received" from "connection lost". Returns FSDB_CONNECTION_DISCONNECTED for a
// null handle or before subscribing.
FSDB_CGO_API int32_t GetConnectionState(FsdbWrapperHandle handle);

FSDB_CGO_API int32_t HasStateSubscription(FsdbWrapperHandle handle);
FSDB_CGO_API int32_t HasStatsSubscription(FsdbWrapperHandle handle);
FSDB_CGO_API int32_t HasStatePathSubscription(FsdbWrapperHandle handle);
FSDB_CGO_API const char* GetClientId(FsdbWrapperHandle handle);

FSDB_CGO_API int32_t WaitForStateUpdates(
    FsdbWrapperHandle handle,
    FsdbPortStateUpdate* out,
    int32_t max_count);
FSDB_CGO_API int32_t WaitForStatsUpdates(
    FsdbWrapperHandle handle,
    FsdbStatsUpdate* out,
    int32_t max_count);
FSDB_CGO_API int32_t WaitForStatePathUpdates(
    FsdbWrapperHandle handle,
    FsdbStatePathUpdate* out,
    int32_t max_count);

FSDB_CGO_API void FreeStateUpdates(FsdbWrapperHandle handle);
FSDB_CGO_API void FreeStatsUpdates(FsdbWrapperHandle handle);
FSDB_CGO_API void FreeStatePathUpdates(FsdbWrapperHandle handle);

#endif // FBOSS_FSDB_CLIENT_CGO_FSDB_CGO_API_H
