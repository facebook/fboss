// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

package fsdb_cgo_client

import (
	"fmt"
	"time"

	"fboss/fsdb/client/test/go_serialized/cgo_subscriber"
)

// SubscriptionType identifies which subscription type to query
type SubscriptionType int32

const (
	PathSubscription  SubscriptionType = 0
	DeltaSubscription SubscriptionType = 1
	PatchSubscription SubscriptionType = 2
)

// FsdbSubscriber wraps the CGO subscriber client
type FsdbSubscriber struct {
	client *cgo_subscriber.FsdbCgoSubscriber
}

// NewFsdbSubscriber creates a new FSDB subscriber
func NewFsdbSubscriber(clientID, host string, port int32) *FsdbSubscriber {
	client := cgo_subscriber.NewFsdbCgoSubscriber(clientID, host, port)
	return &FsdbSubscriber{
		client: client,
	}
}

// Init initializes the subscriber (placeholder for future test harness setup)
func (s *FsdbSubscriber) Init() error {
	if err := s.client.Init(); err != nil {
		return fmt.Errorf("failed to initialize subscriber: %w", err)
	}
	return nil
}

// Cleanup cleans up the subscriber and unsubscribes from all subscriptions
func (s *FsdbSubscriber) Cleanup() error {
	if err := s.client.Cleanup(); err != nil {
		return fmt.Errorf("failed to cleanup subscriber: %w", err)
	}
	return nil
}

// SubscribeStatePath subscribes to a state path (via FsdbPubSubManager)
func (s *FsdbSubscriber) SubscribeStatePath(path []string) error {
	if err := s.client.SubscribeStatePath(path); err != nil {
		return fmt.Errorf("failed to subscribe to state path: %w", err)
	}
	return nil
}

// SubscribeStatPath subscribes to a stat path (via FsdbPubSubManager)
func (s *FsdbSubscriber) SubscribeStatPath(path []string) error {
	if err := s.client.SubscribeStatPath(path); err != nil {
		return fmt.Errorf("failed to subscribe to stat path: %w", err)
	}
	return nil
}

// UnsubscribeStatePath unsubscribes from state path
func (s *FsdbSubscriber) UnsubscribeStatePath() error {
	if err := s.client.UnsubscribeStatePath(); err != nil {
		return fmt.Errorf("failed to unsubscribe from state path: %w", err)
	}
	return nil
}

// UnsubscribeStatPath unsubscribes from stat path
func (s *FsdbSubscriber) UnsubscribeStatPath() error {
	if err := s.client.UnsubscribeStatPath(); err != nil {
		return fmt.Errorf("failed to unsubscribe from stat path: %w", err)
	}
	return nil
}

// SubscribeStateDelta subscribes to a state delta (via FsdbPubSubManager)
func (s *FsdbSubscriber) SubscribeStateDelta(path []string) error {
	if err := s.client.SubscribeStateDelta(path); err != nil {
		return fmt.Errorf("failed to subscribe to state delta: %w", err)
	}
	return nil
}

// SubscribeStatDelta subscribes to a stat delta (via FsdbPubSubManager)
func (s *FsdbSubscriber) SubscribeStatDelta(path []string) error {
	if err := s.client.SubscribeStatDelta(path); err != nil {
		return fmt.Errorf("failed to subscribe to stat delta: %w", err)
	}
	return nil
}

// UnsubscribeStateDelta unsubscribes from state delta
func (s *FsdbSubscriber) UnsubscribeStateDelta() error {
	if err := s.client.UnsubscribeStateDelta(); err != nil {
		return fmt.Errorf("failed to unsubscribe from state delta: %w", err)
	}
	return nil
}

// UnsubscribeStatDelta unsubscribes from stat delta
func (s *FsdbSubscriber) UnsubscribeStatDelta() error {
	if err := s.client.UnsubscribeStatDelta(); err != nil {
		return fmt.Errorf("failed to unsubscribe from stat delta: %w", err)
	}
	return nil
}

// SubscribeStatePatch subscribes to a state or stats patch (via FsdbSubManager)
func (s *FsdbSubscriber) SubscribeStatePatch(paths [][]string, isStats bool) error {
	if err := s.client.SubscribeStatePatch(paths, isStats); err != nil {
		return fmt.Errorf("failed to subscribe to patch: %w", err)
	}
	return nil
}

// UnsubscribeStatePatch unsubscribes from patch
func (s *FsdbSubscriber) UnsubscribeStatePatch() error {
	if err := s.client.UnsubscribeStatePatch(); err != nil {
		return fmt.Errorf("failed to unsubscribe from patch: %w", err)
	}
	return nil
}

// UnsubscribeAll unsubscribes from all subscriptions
func (s *FsdbSubscriber) UnsubscribeAll() error {
	if err := s.client.UnsubscribeAll(); err != nil {
		return fmt.Errorf("failed to unsubscribe all: %w", err)
	}
	return nil
}

// GetSubscriptionState returns the current subscription state for the given type
func (s *FsdbSubscriber) GetSubscriptionState(subType SubscriptionType) (int32, error) {
	state, err := s.client.GetSubscriptionState(int32(subType))
	if err != nil {
		return 0, fmt.Errorf("failed to get subscription state: %w", err)
	}
	return state, nil
}

// IsInitialSyncComplete returns whether initial sync is complete for the given subscription type
func (s *FsdbSubscriber) IsInitialSyncComplete(subType SubscriptionType) (bool, error) {
	complete, err := s.client.IsInitialSyncComplete(int32(subType))
	if err != nil {
		return false, fmt.Errorf("failed to check initial sync: %w", err)
	}
	return complete, nil
}

// WaitForUpdate blocks until an update is received or timeout expires
func (s *FsdbSubscriber) WaitForUpdate(timeout time.Duration) (bool, error) {
	timeoutMs := int64(timeout / time.Millisecond)
	received, err := s.client.WaitForUpdate(timeoutMs)
	if err != nil {
		return false, fmt.Errorf("failed to wait for update: %w", err)
	}
	return received, nil
}

// GetPathUpdateCount returns the number of path updates received
func (s *FsdbSubscriber) GetPathUpdateCount() (int64, error) {
	count, err := s.client.GetPathUpdateCount()
	if err != nil {
		return 0, fmt.Errorf("failed to get path update count: %w", err)
	}
	return count, nil
}

// GetDeltaUpdateCount returns the number of delta updates received
func (s *FsdbSubscriber) GetDeltaUpdateCount() (int64, error) {
	count, err := s.client.GetDeltaUpdateCount()
	if err != nil {
		return 0, fmt.Errorf("failed to get delta update count: %w", err)
	}
	return count, nil
}

// GetPatchUpdateCount returns the number of patch updates received
func (s *FsdbSubscriber) GetPatchUpdateCount() (int64, error) {
	count, err := s.client.GetPatchUpdateCount()
	if err != nil {
		return 0, fmt.Errorf("failed to get patch update count: %w", err)
	}
	return count, nil
}

// GetLastPathUpdateContent returns the content of the last path update
func (s *FsdbSubscriber) GetLastPathUpdateContent() (string, error) {
	content, err := s.client.GetLastPathUpdateContent()
	if err != nil {
		return "", fmt.Errorf("failed to get last path update content: %w", err)
	}
	return content, nil
}

// GetLastDeltaUpdateContent returns the content of the last delta update
func (s *FsdbSubscriber) GetLastDeltaUpdateContent() (string, error) {
	content, err := s.client.GetLastDeltaUpdateContent()
	if err != nil {
		return "", fmt.Errorf("failed to get last delta update content: %w", err)
	}
	return content, nil
}

// GetLastPatchUpdateContent returns the content of the last patch update
func (s *FsdbSubscriber) GetLastPatchUpdateContent() (string, error) {
	content, err := s.client.GetLastPatchUpdateContent()
	if err != nil {
		return "", fmt.Errorf("failed to get last patch update content: %w", err)
	}
	return content, nil
}
