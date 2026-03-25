// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

// Package main demonstrates how to use FsdbCgoPubSubWrapper from Go via CGO
package main

/*
#cgo CXXFLAGS: -std=c++20
#cgo LDFLAGS: -L${SRCDIR} -lfsdb_cgo_pub_sub_wrapper

#include <stdlib.h>

// C++ wrapper around FsdbCgoPubSubWrapper for CGO
// This would be implemented in a separate C wrapper file

typedef void* FsdbWrapperHandle;

extern FsdbWrapperHandle CreateFsdbWrapper(const char* clientId);
extern void DestroyFsdbWrapper(FsdbWrapperHandle handle);
extern void SubscribeToPortMaps(FsdbWrapperHandle handle);
extern void SubscribeToStatsPath(FsdbWrapperHandle handle);
extern int HasStateSubscription(FsdbWrapperHandle handle);
extern int HasStatsSubscription(FsdbWrapperHandle handle);
extern const char* GetClientId(FsdbWrapperHandle handle);
*/
import "C"
import (
	"fmt"
	"log"
	"time"
	"unsafe"
)

// FsdbWrapper wraps the C++ FsdbCgoPubSubWrapper for use in Go
type FsdbWrapper struct {
	handle C.FsdbWrapperHandle
}

// NewFsdbWrapper creates a new FSDB CGO wrapper
// Note: This uses default connection settings (no port specified)
func NewFsdbWrapper(clientID string) (*FsdbWrapper, error) {
	cClientID := C.CString(clientID)
	defer C.free(unsafe.Pointer(cClientID))

	handle := C.CreateFsdbWrapper(cClientID)
	if handle == nil {
		return nil, fmt.Errorf("failed to create FsdbWrapper")
	}

	return &FsdbWrapper{handle: handle}, nil
}

// Close destroys the C++ wrapper
func (w *FsdbWrapper) Close() {
	if w.handle != nil {
		C.DestroyFsdbWrapper(w.handle)
		w.handle = nil
	}
}

// SubscribeToPortMaps subscribes to portMaps state updates
// Uses default FSDB connection (no port specified)
func (w *FsdbWrapper) SubscribeToPortMaps() error {
	C.SubscribeToPortMaps(w.handle)
	// Check if subscription succeeded by verifying status
	if !w.HasStateSubscription() {
		return fmt.Errorf("failed to subscribe to portMaps")
	}
	return nil
}

// SubscribeToStatsPath subscribes to stats path (hardcoded to "agent")
// Uses default FSDB connection (no port specified)
func (w *FsdbWrapper) SubscribeToStatsPath() error {
	C.SubscribeToStatsPath(w.handle)
	// Check if subscription succeeded by verifying status
	if !w.HasStatsSubscription() {
		return fmt.Errorf("failed to subscribe to stats path")
	}
	return nil
}

// HasStateSubscription checks if state subscription is active
func (w *FsdbWrapper) HasStateSubscription() bool {
	return int(C.HasStateSubscription(w.handle)) != 0
}

// HasStatsSubscription checks if stats subscription is active
func (w *FsdbWrapper) HasStatsSubscription() bool {
	return int(C.HasStatsSubscription(w.handle)) != 0
}

// GetClientID returns the client ID
func (w *FsdbWrapper) GetClientID() string {
	cStr := C.GetClientId(w.handle)
	return C.GoString(cStr)
}

// Example usage
func main() {
	// Create FSDB wrapper (uses default connection settings)
	wrapper, err := NewFsdbWrapper("go-example-client")
	if err != nil {
		log.Fatalf("Failed to create wrapper: %v", err)
	}
	defer wrapper.Close()

	log.Printf("Created FSDB wrapper with client ID: %s", wrapper.GetClientID())

	// Subscribe to portMaps state (no port specified - uses default)
	err = wrapper.SubscribeToPortMaps()
	if err != nil {
		log.Fatalf("Failed to subscribe to portMaps: %v", err)
	}
	log.Println("Subscribed to portMaps state")

	// Wait a bit for subscription to establish
	time.Sleep(2 * time.Second)

	// Check subscription status
	if wrapper.HasStateSubscription() {
		log.Println("State subscription is active")
	}

	// Subscribe to stats (hardcoded to "agent" path - no parameter needed)
	err = wrapper.SubscribeToStatsPath()
	if err != nil {
		log.Fatalf("Failed to subscribe to stats: %v", err)
	}
	log.Println("Subscribed to stats path")

	// Check stats subscription status
	if wrapper.HasStatsSubscription() {
		log.Println("Stats subscription is active")
	}

	// In a real application, you would:
	// 1. Start goroutines to call WaitForStateUpdates()/WaitForStatsUpdates()
	// 2. Process each update as it arrives
	// 3. Handle reconnections and errors gracefully

	// Example pattern:
	// go func() {
	//     for {
	//         updates, err := wrapper.WaitForStateUpdates()
	//         if err != nil {
	//             log.Printf("Error getting state update: %v", err)
	//             continue
	//         }
	//         for key, state := range updates {
	//             processStateUpdate(key, state)
	//         }
	//     }
	// }()

	// Keep running for demo
	log.Println("Press Ctrl+C to exit")
	select {}
}
