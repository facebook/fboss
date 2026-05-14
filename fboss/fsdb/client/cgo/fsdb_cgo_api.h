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
#define FSDB_CGO_ABI_VERSION 1

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
// NOLINTEND(modernize-use-using)

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

FSDB_CGO_API void SubscribeToPortMaps(FsdbWrapperHandle handle);
FSDB_CGO_API void SubscribeToPortMapsWithPort(
    FsdbWrapperHandle handle,
    int32_t server_port);

FSDB_CGO_API void SubscribeToStatsPath(
    FsdbWrapperHandle handle,
    const char** path_tokens,
    int32_t num_tokens);
FSDB_CGO_API void SubscribeToStatsPathWithPort(
    FsdbWrapperHandle handle,
    const char** path_tokens,
    int32_t num_tokens,
    int32_t server_port);

FSDB_CGO_API int32_t HasStateSubscription(FsdbWrapperHandle handle);
FSDB_CGO_API int32_t HasStatsSubscription(FsdbWrapperHandle handle);
FSDB_CGO_API const char* GetClientId(FsdbWrapperHandle handle);

FSDB_CGO_API int32_t WaitForStateUpdates(
    FsdbWrapperHandle handle,
    FsdbPortStateUpdate* out,
    int32_t max_count);
FSDB_CGO_API int32_t WaitForStatsUpdates(
    FsdbWrapperHandle handle,
    FsdbStatsUpdate* out,
    int32_t max_count);

FSDB_CGO_API void FreeStateUpdates(FsdbWrapperHandle handle);
FSDB_CGO_API void FreeStatsUpdates(FsdbWrapperHandle handle);

#endif // FBOSS_FSDB_CLIENT_CGO_FSDB_CGO_API_H
