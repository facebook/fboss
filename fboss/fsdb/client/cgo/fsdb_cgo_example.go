// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

// CGO test client for FsdbCgoPubSubWrapper.
// CLI: subscribeToPortState | subscribeToStats | subscribeToPortStateAndStats.
// Build: `go build -o fsdb_cgo_example fsdb_cgo_example.go` (needs the C++ .a alongside).
package main

/*
#cgo CXXFLAGS: -std=c++20
#cgo CFLAGS: -I${SRCDIR}
#cgo LDFLAGS: -L${SRCDIR} -lfsdb_cgo_pub_sub_wrapper -lstdc++ -lm -lpthread -ldl

#include <stdlib.h>
#include "fsdb_cgo_api.h"
*/
import "C"

import (
	"flag"
	"fmt"
	"log"
	"os"
	"os/signal"
	"sync"
	"syscall"
	"unsafe"
)

type PortStateUpdate struct {
	PortName  string
	PortID    int32
	OperState bool
}

type StatsUpdate struct {
	Key      string
	Data     []byte // raw Thrift-serialized OperState contents
	Protocol int32  // OperProtocol: BINARY=1, COMPACT=2, SIMPLE_JSON=3
}

type StatePathUpdate struct {
	Key      string
	Data     []byte
	Protocol int32
}

type FsdbCgoClient struct {
	handle     C.FsdbWrapperHandle
	serverPort int
}

// Idempotent. Returns an error on ABI mismatch or init failure.
func Init() error {
	rc := int32(C.FsdbInit(C.int32_t(C.FSDB_CGO_ABI_VERSION)))
	switch rc {
	case 0:
		return nil
	case 2:
		return fmt.Errorf(
			"fsdb_cgo ABI mismatch: header=%d library=%d",
			int32(C.FSDB_CGO_ABI_VERSION), int32(C.FsdbCgoAbiVersion()))
	default:
		return fmt.Errorf("FsdbInit returned %d", rc)
	}
}

func NewFsdbCgoClient(clientID string, serverPort int) (*FsdbCgoClient, error) {
	cClientID := C.CString(clientID)
	defer C.free(unsafe.Pointer(cClientID))

	handle := C.CreateFsdbWrapper(cClientID)
	if handle == nil {
		return nil, fmt.Errorf("CreateFsdbWrapper returned nil for clientID=%q", clientID)
	}
	return &FsdbCgoClient{handle: handle, serverPort: serverPort}, nil
}

// Wakes parked WaitFor* within ~100 ms. Call BEFORE Close.
// Logs a warning if the C side returns non-zero (caught exception); failure
// here means the watcher goroutines may stay parked and wg.Wait() will hang.
func (c *FsdbCgoClient) Shutdown() {
	if c.handle == nil {
		return
	}
	if rc := int32(C.ShutdownFsdbWrapper(c.handle)); rc != 0 {
		log.Printf("fsdb: ShutdownFsdbWrapper returned %d; watcher goroutines may not unblock", rc)
	}
}

// Use-after-free if any goroutine is still parked in WaitFor*.
func (c *FsdbCgoClient) Close() {
	if c.handle != nil {
		C.DestroyFsdbWrapper(c.handle)
		c.handle = nil
	}
}

// Subscribes to portMaps. nil host => localhost; serverPort < 0 => default port.
func (c *FsdbCgoClient) SubscribePortState() {
	C.SubscribeToPortMaps(c.handle, nil, C.int32_t(c.serverPort))
}

// SubscribeStatsPath subscribes to an arbitrary FSDB stats path (localhost, on
// the serverPort the client was created with). Single subscription per client;
// further calls log a WARNING on the C++ side and no-op.
func (c *FsdbCgoClient) SubscribeStatsPath(path []string) {
	if len(path) == 0 {
		return
	}
	cTokens := make([]*C.char, len(path))
	for i, tok := range path {
		cTokens[i] = C.CString(tok)
		defer C.free(unsafe.Pointer(cTokens[i]))
	}
	C.SubscribeToStatsPath(
		c.handle, &cTokens[0], C.int32_t(len(path)), nil, C.int32_t(c.serverPort))
}

// SubscribeStatePath subscribes to an arbitrary FSDB state path. Distinct
// from SubscribePortState (which is the typed PortMaps subscription). Single
// subscription per client.
func (c *FsdbCgoClient) SubscribeStatePath(path []string) {
	if len(path) == 0 {
		return
	}
	cTokens := make([]*C.char, len(path))
	for i, tok := range path {
		cTokens[i] = C.CString(tok)
		defer C.free(unsafe.Pointer(cTokens[i]))
	}
	C.SubscribeToStatePath(
		c.handle, &cTokens[0], C.int32_t(len(path)), nil, C.int32_t(c.serverPort))
}

func (c *FsdbCgoClient) HasStateSubscription() bool {
	return int(C.HasStateSubscription(c.handle)) != 0
}

func (c *FsdbCgoClient) HasStatsSubscription() bool {
	return int(C.HasStatsSubscription(c.handle)) != 0
}

func (c *FsdbCgoClient) HasStatePathSubscription() bool {
	return int(C.HasStatePathSubscription(c.handle)) != 0
}

func (c *FsdbCgoClient) GetClientID() string {
	return C.GoString(C.GetClientId(c.handle))
}

// Empty slice (nil error) on Shutdown.
func (c *FsdbCgoClient) WaitForPortStateUpdates(maxUpdates int) ([]PortStateUpdate, error) {
	if maxUpdates <= 0 {
		return nil, fmt.Errorf("maxUpdates must be positive, got %d", maxUpdates)
	}
	out := make([]C.FsdbPortStateUpdate, maxUpdates)
	count := int(C.WaitForStateUpdates(c.handle, &out[0], C.int32_t(maxUpdates)))
	// Always release any partial buffer the C side may have populated, even on
	// error returns (count < 0). FreeStateUpdates is a no-op when nothing's
	// allocated, so the unconditional defer is safe.
	defer C.FreeStateUpdates(c.handle)
	if count < 0 {
		return nil, fmt.Errorf("WaitForStateUpdates failed (returned %d)", count)
	}
	updates := make([]PortStateUpdate, count)
	for i := range count {
		updates[i] = PortStateUpdate{
			PortName:  C.GoString(out[i].port_name),
			PortID:    int32(out[i].port_id),
			OperState: int(out[i].oper_state) != 0,
		}
	}
	return updates, nil
}

// WaitForStatsUpdates blocks until at least one stats update is available,
// then returns up to maxUpdates entries with raw Thrift-serialized bytes.
// Empty slice (nil error) on Shutdown.
func (c *FsdbCgoClient) WaitForStatsUpdates(maxUpdates int) ([]StatsUpdate, error) {
	if maxUpdates <= 0 {
		return nil, fmt.Errorf("maxUpdates must be positive, got %d", maxUpdates)
	}
	out := make([]C.FsdbStatsUpdate, maxUpdates)
	count := int(C.WaitForStatsUpdates(c.handle, &out[0], C.int32_t(maxUpdates)))
	// Same defer rationale as WaitForPortStateUpdates above.
	defer C.FreeStatsUpdates(c.handle)
	if count < 0 {
		return nil, fmt.Errorf("WaitForStatsUpdates failed (returned %d)", count)
	}
	updates := make([]StatsUpdate, count)
	for i := range count {
		updates[i] = StatsUpdate{
			Key:      C.GoString(out[i].key),
			Data:     C.GoBytes(unsafe.Pointer(out[i].data), out[i].data_len),
			Protocol: int32(out[i].protocol),
		}
	}
	return updates, nil
}

func (c *FsdbCgoClient) WaitForStatePathUpdates(maxUpdates int) ([]StatePathUpdate, error) {
	if maxUpdates <= 0 {
		return nil, fmt.Errorf("maxUpdates must be positive, got %d", maxUpdates)
	}
	out := make([]C.FsdbStatePathUpdate, maxUpdates)
	count := int(C.WaitForStatePathUpdates(c.handle, &out[0], C.int32_t(maxUpdates)))
	defer C.FreeStatePathUpdates(c.handle)
	if count < 0 {
		return nil, fmt.Errorf("WaitForStatePathUpdates failed (returned %d)", count)
	}
	updates := make([]StatePathUpdate, count)
	for i := range count {
		updates[i] = StatePathUpdate{
			Key:      C.GoString(out[i].key),
			Data:     C.GoBytes(unsafe.Pointer(out[i].data), out[i].data_len),
			Protocol: int32(out[i].protocol),
		}
	}
	return updates, nil
}

const maxUpdatesPerBatch = 128

func subscribeToPortState(client *FsdbCgoClient, done <-chan struct{}) {
	log.Println("[subscribeToPortState] waiting for port-state updates …")
	for {
		select {
		case <-done:
			return
		default:
		}
		updates, err := client.WaitForPortStateUpdates(maxUpdatesPerBatch)
		if err != nil {
			log.Printf("[subscribeToPortState] error: %v", err)
			return
		}
		for _, u := range updates {
			state := "DOWN"
			if u.OperState {
				state = "UP"
			}
			log.Printf("[subscribeToPortState] port=%s  port_id=%d  oper_state=%s", u.PortName, u.PortID, state)
		}
	}
}

func subscribeToStats(client *FsdbCgoClient, done <-chan struct{}) {
	log.Println("[subscribeToStats] waiting for stats updates …")
	for {
		select {
		case <-done:
			return
		default:
		}
		updates, err := client.WaitForStatsUpdates(maxUpdatesPerBatch)
		if err != nil {
			log.Printf("[subscribeToStats] error: %v", err)
			return
		}
		for _, u := range updates {
			log.Printf("[subscribeToStats] key=%s  data_len=%d bytes", u.Key, len(u.Data))
		}
	}
}

func subscribeToStatePath(client *FsdbCgoClient, done <-chan struct{}) {
	log.Println("[subscribeToStatePath] waiting for state-path updates …")
	for {
		select {
		case <-done:
			return
		default:
		}
		updates, err := client.WaitForStatePathUpdates(maxUpdatesPerBatch)
		if err != nil {
			log.Printf("[subscribeToStatePath] error: %v", err)
			return
		}
		for _, u := range updates {
			log.Printf("[subscribeToStatePath] key=%s  data_len=%d bytes  protocol=%d",
				u.Key, len(u.Data), u.Protocol)
		}
	}
}

func usage() {
	fmt.Fprintf(os.Stderr, "Usage: %s [flags] <subcommand>\n\n", os.Args[0])
	fmt.Fprintln(os.Stderr, "Subcommands:")
	fmt.Fprintln(os.Stderr, "  subscribeToPortState           Subscribe to port-state changes (typed)")
	fmt.Fprintln(os.Stderr, "  subscribeToStats               Subscribe to stats path ['agent']")
	fmt.Fprintln(os.Stderr, "  subscribeToStatePath           Subscribe to state path ['agent','switchState','interfaceMap']")
	fmt.Fprintln(os.Stderr, "  subscribeToAll                 All three subscriptions concurrently")
	fmt.Fprintln(os.Stderr, "\nFlags:")
	flag.PrintDefaults()
}

func main() {
	port := flag.Int("port", 5908, "FSDB server port (use -1 for default)")
	clientID := flag.String("client-id", "go-cgo-test-client", "Client ID for FSDB connection")
	flag.Usage = usage
	flag.Parse()

	if flag.NArg() < 1 {
		usage()
		os.Exit(1)
	}
	subcmd := flag.Arg(0)

	if err := Init(); err != nil {
		log.Fatalf("FsdbInit failed: %v", err)
	}

	client, err := NewFsdbCgoClient(*clientID, *port)
	if err != nil {
		log.Fatalf("Failed to create FSDB client: %v", err)
	}
	log.Printf("Created FSDB client (id=%s, port=%d)", client.GetClientID(), *port)

	// `done` is closed by either (a) Ctrl-C / SIGTERM, or (b) all watcher
	// goroutines exiting on error. Without (b), main would block on <-done
	// forever even when no watchers remained, and the C++ wrapper would never
	// be torn down. sync.Once protects against double-close panic.
	sigCh := make(chan os.Signal, 1)
	signal.Notify(sigCh, syscall.SIGINT, syscall.SIGTERM)
	done := make(chan struct{})
	var doneOnce sync.Once
	closeDone := func() { doneOnce.Do(func() { close(done) }) }

	go func() {
		sig := <-sigCh
		log.Printf("Received %v, shutting down …", sig)
		closeDone()
	}()

	subscribe := func(portState, stats, statePath bool) {
		if portState {
			client.SubscribePortState()
			log.Println("Subscribed to port-state (portMaps)")
		}
		if stats {
			// Example: ["agent"] gives AgentStats. Vendors can pass any path.
			client.SubscribeStatsPath([]string{"agent"})
			log.Println("Subscribed to stats path agent")
		}
		if statePath {
			// Example: any state path. Vendors decode the raw bytes themselves.
			client.SubscribeStatePath([]string{"agent", "switchState", "interfaceMap"})
			log.Println("Subscribed to state path agent.switchState.interfaceMap")
		}
	}

	var wg sync.WaitGroup

	switch subcmd {
	case "subscribeToPortState":
		subscribe(true, false, false)
		wg.Go(func() { subscribeToPortState(client, done) })

	case "subscribeToStats":
		subscribe(false, true, false)
		wg.Go(func() { subscribeToStats(client, done) })

	case "subscribeToStatePath":
		subscribe(false, false, true)
		wg.Go(func() { subscribeToStatePath(client, done) })

	case "subscribeToAll":
		subscribe(true, true, true)
		wg.Go(func() { subscribeToPortState(client, done) })
		wg.Go(func() { subscribeToStats(client, done) })
		wg.Go(func() { subscribeToStatePath(client, done) })

	default:
		fmt.Fprintf(os.Stderr, "Unknown subcommand: %s\n", subcmd)
		usage()
		os.Exit(1)
	}

	// If every watcher exits on its own (e.g. all WaitFor* errored out),
	// signal main to proceed to teardown instead of hanging on <-done.
	go func() {
		wg.Wait()
		log.Println("All watcher goroutines exited; initiating shutdown …")
		closeDone()
	}()

	<-done
	log.Println("Shutting down …")
	// Order matters: Shutdown wakes parked WaitFor*, wg.Wait drains them,
	// then Close is safe (else use-after-free).
	client.Shutdown()
	wg.Wait()
	client.Close()
}
