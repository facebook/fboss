// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

package fsdb_test

import (
	"flag"
	"fmt"
	"testing"
	"time"

	"github.com/stretchr/testify/assert"
	"github.com/stretchr/testify/require"
)

// SubscriptionState values from C++ enum
const (
	StateDisconnected          int32 = 0
	StateDisconnectedGRHold    int32 = 1
	StateDisconnectedGRExpired int32 = 2
	StateCancelled             int32 = 3
	StateConnected             int32 = 4
)

var (
	useInProcessServer = flag.Bool("use_in_process_server", true,
		"Use in-process FsdbTestServer (true) or external FSDB server (false)")
	fsdbHost = flag.String("fsdb_host", "::1", "External FSDB server host IP")
	fsdbPort = flag.Int("fsdb_port", 5908, "External FSDB server port")
)

// isInProcessMode returns true if tests are running with in-process server
func isInProcessMode() bool {
	return *useInProcessServer
}

// setupTestSubscriber creates a subscriber configured for the current server mode
func setupTestSubscriber(t *testing.T) *FsdbTestSubscriber {
	clientID := fmt.Sprintf("test_client_%s_%d", t.Name(), time.Now().UnixNano())

	var opts []Option
	if isInProcessMode() {
		opts = append(opts, WithInProcessServer())
	} else {
		opts = append(opts, WithExternalServer(*fsdbHost, int32(*fsdbPort)))
	}

	sub, err := NewFsdbTestSubscriber(clientID, opts...)
	require.NoError(t, err)

	err = sub.Start()
	require.NoError(t, err)

	t.Cleanup(func() {
		_ = sub.UnsubscribeAll()
		sub.Stop()
	})

	return sub
}

// startPublisher starts and connects the publisher (only for in-process server mode)
// Returns a cleanup function that should be deferred
func startPublisher(t *testing.T, sub *FsdbTestSubscriber) func() {
	t.Helper()

	err := sub.ConnectPublisher()
	require.NoError(t, err)

	ready, err := sub.WaitForPublisherReady(15 * time.Second)
	require.NoError(t, err)
	require.True(t, ready, "publisher should be ready within timeout")

	return func() {
		_ = sub.DisconnectPublisher()
	}
}

// ==============================================================================
// Subscriber-only Tests (work with both in-process and external server modes)
// ==============================================================================

// TestPathSubscription_Connect tests that we can create a path subscription
// without errors. The subscription won't reach CONNECTED state until data
// arrives from a publisher.
func TestPathSubscription_Connect(t *testing.T) {
	sub := setupTestSubscriber(t)

	err := sub.SubscribeStatePath([]string{"agent", "switchState", "portMaps"})
	require.NoError(t, err)

	// Give time for the subscription to be registered
	time.Sleep(2 * time.Second)
}

// TestPathSubscription_Disconnect tests subscribe/unsubscribe lifecycle
func TestPathSubscription_Disconnect(t *testing.T) {
	sub := setupTestSubscriber(t)

	err := sub.SubscribeStatePath([]string{"agent", "switchState", "portMaps"})
	require.NoError(t, err)

	// Give time for the subscription to be registered
	time.Sleep(2 * time.Second)

	err = sub.UnsubscribeStatePath()
	require.NoError(t, err)

	state, err := sub.GetSubscriptionState(PathSubscription)
	require.NoError(t, err)
	assert.Equal(t, StateDisconnected, state)
}

// TestPathSubscription_Reconnect tests that we can reconnect after unsubscribing
func TestPathSubscription_Reconnect(t *testing.T) {
	sub := setupTestSubscriber(t)

	err := sub.SubscribeStatePath([]string{"agent", "switchState", "portMaps"})
	require.NoError(t, err)

	time.Sleep(1 * time.Second)

	err = sub.UnsubscribeStatePath()
	require.NoError(t, err)

	err = sub.SubscribeStatePath([]string{"agent", "switchState", "portMaps"})
	require.NoError(t, err)

	time.Sleep(1 * time.Second)
}

// TestDeltaSubscription_Connect tests that we can create a delta subscription
func TestDeltaSubscription_Connect(t *testing.T) {
	sub := setupTestSubscriber(t)

	err := sub.SubscribeStateDelta([]string{"agent", "switchState", "portMaps"})
	require.NoError(t, err)

	time.Sleep(2 * time.Second)
}

// TestDeltaSubscription_Disconnect tests delta subscribe/unsubscribe lifecycle
func TestDeltaSubscription_Disconnect(t *testing.T) {
	sub := setupTestSubscriber(t)

	err := sub.SubscribeStateDelta([]string{"agent", "switchState", "portMaps"})
	require.NoError(t, err)

	time.Sleep(2 * time.Second)

	err = sub.UnsubscribeStateDelta()
	require.NoError(t, err)

	state, err := sub.GetSubscriptionState(DeltaSubscription)
	require.NoError(t, err)
	assert.Equal(t, StateDisconnected, state)
}

// TestPatchSubscription_Connect tests that we can create a patch subscription
func TestPatchSubscription_Connect(t *testing.T) {
	sub := setupTestSubscriber(t)

	err := sub.SubscribeStatePatch([]string{"agent", "switchState", "portMaps"})
	require.NoError(t, err)

	time.Sleep(2 * time.Second)
}

// TestPatchSubscription_Disconnect tests patch subscribe/unsubscribe lifecycle
func TestPatchSubscription_Disconnect(t *testing.T) {
	sub := setupTestSubscriber(t)

	err := sub.SubscribeStatePatch([]string{"agent", "switchState", "portMaps"})
	require.NoError(t, err)

	time.Sleep(2 * time.Second)

	err = sub.UnsubscribeStatePatch()
	require.NoError(t, err)

	state, err := sub.GetSubscriptionState(PatchSubscription)
	require.NoError(t, err)
	assert.Equal(t, StateDisconnected, state)
}

// TestMultipleSubscriptionTypes tests subscribing to all three types simultaneously
func TestMultipleSubscriptionTypes(t *testing.T) {
	sub := setupTestSubscriber(t)

	err := sub.SubscribeStatePath([]string{"agent", "switchState", "portMaps"})
	require.NoError(t, err)

	err = sub.SubscribeStateDelta([]string{"agent", "switchState", "portMaps"})
	require.NoError(t, err)

	err = sub.SubscribeStatePatch([]string{"agent", "switchState", "portMaps"})
	require.NoError(t, err)

	// All subscriptions should be active
	time.Sleep(2 * time.Second)
}

// ==============================================================================
// Publisher Tests (only run in in-process server mode)
// ==============================================================================

// TestPublisher_Connect tests that we can connect and disconnect the publisher
func TestPublisher_Connect(t *testing.T) {
	if !isInProcessMode() {
		t.Skip("Publisher tests only run in in-process server mode")
	}

	sub := setupTestSubscriber(t)

	err := sub.ConnectPublisher()
	require.NoError(t, err)

	connected, err := sub.WaitForPublisherConnected(10 * time.Second)
	require.NoError(t, err)
	assert.True(t, connected, "publisher should connect")

	isConnected, err := sub.IsPublisherConnected()
	require.NoError(t, err)
	assert.True(t, isConnected)

	err = sub.DisconnectPublisher()
	require.NoError(t, err)
}

// TestPublisher_Ready tests waiting for publisher to be fully ready
func TestPublisher_Ready(t *testing.T) {
	if !isInProcessMode() {
		t.Skip("Publisher tests only run in in-process server mode")
	}

	sub := setupTestSubscriber(t)

	err := sub.ConnectPublisher()
	require.NoError(t, err)

	ready, err := sub.WaitForPublisherReady(15 * time.Second)
	require.NoError(t, err)
	assert.True(t, ready, "publisher should be ready")

	err = sub.DisconnectPublisher()
	require.NoError(t, err)
}

// TestTypedPublish_AgentSwitchStatePortInfo tests typed publishing to agent.switchState.portMaps
func TestTypedPublish_AgentSwitchStatePortInfo(t *testing.T) {
	if !isInProcessMode() {
		t.Skip("Publisher tests only run in in-process server mode")
	}

	sub := setupTestSubscriber(t)
	defer startPublisher(t, sub)()

	// Publish typed data
	err := sub.PublishAgentSwitchStatePortInfo(0, 1, "eth1/1/1", 1)
	require.NoError(t, err)

	// Give time for the data to be published
	time.Sleep(500 * time.Millisecond)
}

// ==============================================================================
// Data Flow Tests with Publisher (only run in in-process server mode)
// ==============================================================================

// TestPathSubscription_InitialSync tests receiving initial sync with typed published data
func TestPathSubscription_InitialSync(t *testing.T) {
	if !isInProcessMode() {
		t.Skip("Data flow tests with publisher only run in in-process server mode")
	}

	sub := setupTestSubscriber(t)
	defer startPublisher(t, sub)()

	// Subscribe to the root "agent" path FIRST (before publishing)
	err := sub.SubscribeStatePath([]string{"agent"})
	require.NoError(t, err)

	// Give subscription time to establish
	time.Sleep(500 * time.Millisecond)

	// Now publish typed data to agent.switchState.portMaps
	err = sub.PublishAgentSwitchStatePortInfo(0, 1, "eth1/1/1", 1)
	require.NoError(t, err)

	// Wait for the data to be received (check update count increases)
	assert.Eventually(t, func() bool {
		count, _ := sub.GetPathUpdateCount()
		return count > 0
	}, 15*time.Second, 100*time.Millisecond, "should receive at least one path update")

	count, err := sub.GetPathUpdateCount()
	require.NoError(t, err)
	assert.Greater(t, count, int64(0), "should have received at least one update")

	content, err := sub.GetLastPathUpdateContent()
	require.NoError(t, err)
	assert.NotEmpty(t, content, "should have update content")
}

// TestPathSubscription_IncrementalUpdate tests receiving incremental updates with typed published data
func TestPathSubscription_IncrementalUpdate(t *testing.T) {
	if !isInProcessMode() {
		t.Skip("Data flow tests with publisher only run in in-process server mode")
	}

	sub := setupTestSubscriber(t)
	defer startPublisher(t, sub)()

	// Subscribe to the root "agent" path FIRST
	err := sub.SubscribeStatePath([]string{"agent"})
	require.NoError(t, err)

	// Give subscription time to establish
	time.Sleep(500 * time.Millisecond)

	// Publish initial data
	err = sub.PublishAgentSwitchStatePortInfo(0, 1, "eth1/1/1", 0)
	require.NoError(t, err)

	// Wait for initial update using polling
	var initialCount int64
	assert.Eventually(t, func() bool {
		initialCount, _ = sub.GetPathUpdateCount()
		return initialCount > 0
	}, 15*time.Second, 100*time.Millisecond)

	// Wait a bit to ensure state is stable
	time.Sleep(100 * time.Millisecond)

	// Get current count before incremental update
	initialCount, _ = sub.GetPathUpdateCount()

	// Publish an update (port comes up)
	err = sub.PublishAgentSwitchStatePortInfo(0, 1, "eth1/1/1", 1)
	require.NoError(t, err)

	// Wait for the incremental update using polling
	assert.Eventually(t, func() bool {
		count, _ := sub.GetPathUpdateCount()
		return count > initialCount
	}, 5*time.Second, 100*time.Millisecond, "should receive an incremental update")

	newCount, err := sub.GetPathUpdateCount()
	require.NoError(t, err)
	assert.Greater(t, newCount, initialCount, "update count should increase")

	content, err := sub.GetLastPathUpdateContent()
	require.NoError(t, err)
	assert.NotEmpty(t, content, "should have update content")
}

// TestDeltaSubscription_InitialSync tests receiving initial sync for delta subscription with typed data
func TestDeltaSubscription_InitialSync(t *testing.T) {
	if !isInProcessMode() {
		t.Skip("Data flow tests with publisher only run in in-process server mode")
	}

	sub := setupTestSubscriber(t)
	defer startPublisher(t, sub)()

	// Subscribe to the root "agent" path FIRST
	err := sub.SubscribeStateDelta([]string{"agent"})
	require.NoError(t, err)

	// Give subscription time to establish
	time.Sleep(500 * time.Millisecond)

	// Now publish typed data
	err = sub.PublishAgentSwitchStatePortInfo(0, 1, "eth1/1/1", 1)
	require.NoError(t, err)

	// Wait for the data to be received
	assert.Eventually(t, func() bool {
		count, _ := sub.GetDeltaUpdateCount()
		return count > 0
	}, 15*time.Second, 100*time.Millisecond, "should receive at least one delta update")

	count, err := sub.GetDeltaUpdateCount()
	require.NoError(t, err)
	assert.Greater(t, count, int64(0), "should have received at least one delta update")
}

// TestDeltaSubscription_IncrementalUpdate tests receiving incremental delta updates with typed data
func TestDeltaSubscription_IncrementalUpdate(t *testing.T) {
	if !isInProcessMode() {
		t.Skip("Data flow tests with publisher only run in in-process server mode")
	}

	sub := setupTestSubscriber(t)
	defer startPublisher(t, sub)()

	// Subscribe to the root "agent" path FIRST
	err := sub.SubscribeStateDelta([]string{"agent"})
	require.NoError(t, err)

	// Give subscription time to establish
	time.Sleep(500 * time.Millisecond)

	// Publish initial data
	err = sub.PublishAgentSwitchStatePortInfo(0, 1, "eth1/1/1", 0)
	require.NoError(t, err)

	// Wait for initial update using polling
	var initialCount int64
	assert.Eventually(t, func() bool {
		initialCount, _ = sub.GetDeltaUpdateCount()
		return initialCount > 0
	}, 15*time.Second, 100*time.Millisecond)

	// Wait a bit to ensure state is stable
	time.Sleep(100 * time.Millisecond)

	// Get current count before incremental update
	initialCount, _ = sub.GetDeltaUpdateCount()

	// Publish an update
	err = sub.PublishAgentSwitchStatePortInfo(0, 1, "eth1/1/1", 1)
	require.NoError(t, err)

	// Wait for the incremental update using polling
	assert.Eventually(t, func() bool {
		count, _ := sub.GetDeltaUpdateCount()
		return count > initialCount
	}, 5*time.Second, 100*time.Millisecond, "should receive an incremental delta update")

	newCount, err := sub.GetDeltaUpdateCount()
	require.NoError(t, err)
	assert.Greater(t, newCount, initialCount, "delta update count should increase")
}

// TestPatchSubscription_InitialSync tests receiving initial sync for patch subscription with typed data
func TestPatchSubscription_InitialSync(t *testing.T) {
	if !isInProcessMode() {
		t.Skip("Data flow tests with publisher only run in in-process server mode")
	}

	sub := setupTestSubscriber(t)
	defer startPublisher(t, sub)()

	// Subscribe to the root "agent" path FIRST
	err := sub.SubscribeStatePatch([]string{"agent"})
	require.NoError(t, err)

	// Give subscription time to establish
	time.Sleep(500 * time.Millisecond)

	// Now publish typed data
	err = sub.PublishAgentSwitchStatePortInfo(0, 1, "eth1/1/1", 1)
	require.NoError(t, err)

	// Wait for the data to be received
	assert.Eventually(t, func() bool {
		count, _ := sub.GetPatchUpdateCount()
		return count > 0
	}, 15*time.Second, 100*time.Millisecond, "should receive at least one patch update")

	count, err := sub.GetPatchUpdateCount()
	require.NoError(t, err)
	assert.Greater(t, count, int64(0), "should have received at least one patch update")
}

// TestPatchSubscription_IncrementalUpdate tests receiving incremental patch updates with typed data
func TestPatchSubscription_IncrementalUpdate(t *testing.T) {
	if !isInProcessMode() {
		t.Skip("Data flow tests with publisher only run in in-process server mode")
	}

	sub := setupTestSubscriber(t)
	defer startPublisher(t, sub)()

	// Subscribe to the root "agent" path FIRST
	err := sub.SubscribeStatePatch([]string{"agent"})
	require.NoError(t, err)

	// Give subscription time to establish
	time.Sleep(500 * time.Millisecond)

	// Publish initial data
	err = sub.PublishAgentSwitchStatePortInfo(0, 1, "eth1/1/1", 0)
	require.NoError(t, err)

	// Wait for initial update using polling
	var initialCount int64
	assert.Eventually(t, func() bool {
		initialCount, _ = sub.GetPatchUpdateCount()
		return initialCount > 0
	}, 15*time.Second, 100*time.Millisecond)

	// Wait a bit to ensure state is stable
	time.Sleep(100 * time.Millisecond)

	// Get current count before incremental update
	initialCount, _ = sub.GetPatchUpdateCount()

	// Publish an update
	err = sub.PublishAgentSwitchStatePortInfo(0, 1, "eth1/1/1", 1)
	require.NoError(t, err)

	// Wait for the incremental update using polling
	assert.Eventually(t, func() bool {
		count, _ := sub.GetPatchUpdateCount()
		return count > initialCount
	}, 5*time.Second, 100*time.Millisecond, "should receive an incremental patch update")

	newCount, err := sub.GetPatchUpdateCount()
	require.NoError(t, err)
	assert.Greater(t, newCount, initialCount, "patch update count should increase")
}
