// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

package fsdb_test

import (
	"fmt"
	"time"

	"fboss/fsdb/client/test/go_serialized/test_cgo_client"
)

// SubscriptionType identifies which subscription type to query
type SubscriptionType int32

const (
	PathSubscription  SubscriptionType = 0
	DeltaSubscription SubscriptionType = 1
	PatchSubscription SubscriptionType = 2
)

// FsdbTestSubscriber wraps the CGO test client
type FsdbTestSubscriber struct {
	client    *test_cgo_client.FsdbTestCgoClient
	inProcess bool
	hostIP    string
	fsdbPort  int32
}

// Option is a configuration option for FsdbTestSubscriber
type Option func(*FsdbTestSubscriber)

// WithInProcessServer configures the subscriber to use an in-process FSDB test server
func WithInProcessServer() Option {
	return func(s *FsdbTestSubscriber) {
		s.inProcess = true
	}
}

// WithExternalServer configures the subscriber to connect to an external FSDB server
func WithExternalServer(hostIP string, port int32) Option {
	return func(s *FsdbTestSubscriber) {
		s.inProcess = false
		s.hostIP = hostIP
		s.fsdbPort = port
	}
}

// NewFsdbTestSubscriber creates a new FSDB test subscriber
func NewFsdbTestSubscriber(clientID string, opts ...Option) (*FsdbTestSubscriber, error) {
	client := test_cgo_client.NewFsdbTestCgoClient(clientID)

	s := &FsdbTestSubscriber{
		client:    client,
		inProcess: true,
		hostIP:    "::1",
		fsdbPort:  5908,
	}

	for _, opt := range opts {
		opt(s)
	}

	return s, nil
}

// Start starts the subscriber (launches in-process server if configured)
func (s *FsdbTestSubscriber) Start() error {
	if s.inProcess {
		if err := s.client.StartInProcessServer(); err != nil {
			return fmt.Errorf("failed to start in-process server: %w", err)
		}
		port, err := s.client.GetServerPort()
		if err != nil {
			return fmt.Errorf("failed to get server port: %w", err)
		}
		s.fsdbPort = port
	} else {
		if err := s.client.SetServer(s.hostIP, s.fsdbPort); err != nil {
			return fmt.Errorf("failed to set server: %w", err)
		}
	}
	return nil
}

// Stop stops the subscriber (stops in-process server if running)
func (s *FsdbTestSubscriber) Stop() {
	if s.inProcess {
		_ = s.client.StopInProcessServer()
	}
}

// SubscribeStatePath subscribes to a state path (via FsdbPubSubManager)
func (s *FsdbTestSubscriber) SubscribeStatePath(path []string) error {
	if err := s.client.SubscribeStatePath(path); err != nil {
		return fmt.Errorf("failed to subscribe to state path: %w", err)
	}
	return nil
}

// UnsubscribeStatePath unsubscribes from state path
func (s *FsdbTestSubscriber) UnsubscribeStatePath() error {
	if err := s.client.UnsubscribeStatePath(); err != nil {
		return fmt.Errorf("failed to unsubscribe from state path: %w", err)
	}
	return nil
}

// SubscribeStateDelta subscribes to a state delta (via FsdbPubSubManager)
func (s *FsdbTestSubscriber) SubscribeStateDelta(path []string) error {
	if err := s.client.SubscribeStateDelta(path); err != nil {
		return fmt.Errorf("failed to subscribe to state delta: %w", err)
	}
	return nil
}

// UnsubscribeStateDelta unsubscribes from state delta
func (s *FsdbTestSubscriber) UnsubscribeStateDelta() error {
	if err := s.client.UnsubscribeStateDelta(); err != nil {
		return fmt.Errorf("failed to unsubscribe from state delta: %w", err)
	}
	return nil
}

// SubscribeStatePatch subscribes to a state patch (via FsdbSubManager)
func (s *FsdbTestSubscriber) SubscribeStatePatch(path []string) error {
	if err := s.client.SubscribeStatePatch(path); err != nil {
		return fmt.Errorf("failed to subscribe to state patch: %w", err)
	}
	return nil
}

// UnsubscribeStatePatch unsubscribes from state patch
func (s *FsdbTestSubscriber) UnsubscribeStatePatch() error {
	if err := s.client.UnsubscribeStatePatch(); err != nil {
		return fmt.Errorf("failed to unsubscribe from state patch: %w", err)
	}
	return nil
}

// UnsubscribeAll unsubscribes from all subscriptions
func (s *FsdbTestSubscriber) UnsubscribeAll() error {
	if err := s.client.UnsubscribeAll(); err != nil {
		return fmt.Errorf("failed to unsubscribe all: %w", err)
	}
	return nil
}

// GetSubscriptionState returns the current subscription state for the given type
func (s *FsdbTestSubscriber) GetSubscriptionState(subType SubscriptionType) (int32, error) {
	state, err := s.client.GetSubscriptionState(int32(subType))
	if err != nil {
		return 0, fmt.Errorf("failed to get subscription state: %w", err)
	}
	return state, nil
}

// IsInitialSyncComplete returns whether initial sync is complete for the given subscription type
func (s *FsdbTestSubscriber) IsInitialSyncComplete(subType SubscriptionType) (bool, error) {
	complete, err := s.client.IsInitialSyncComplete(int32(subType))
	if err != nil {
		return false, fmt.Errorf("failed to check initial sync: %w", err)
	}
	return complete, nil
}

// WaitForUpdate blocks until an update is received or timeout expires
func (s *FsdbTestSubscriber) WaitForUpdate(timeout time.Duration) (bool, error) {
	timeoutMs := int64(timeout / time.Millisecond)
	received, err := s.client.WaitForUpdate(timeoutMs)
	if err != nil {
		return false, fmt.Errorf("failed to wait for update: %w", err)
	}
	return received, nil
}

// GetPathUpdateCount returns the number of path updates received
func (s *FsdbTestSubscriber) GetPathUpdateCount() (int64, error) {
	count, err := s.client.GetPathUpdateCount()
	if err != nil {
		return 0, fmt.Errorf("failed to get path update count: %w", err)
	}
	return count, nil
}

// GetDeltaUpdateCount returns the number of delta updates received
func (s *FsdbTestSubscriber) GetDeltaUpdateCount() (int64, error) {
	count, err := s.client.GetDeltaUpdateCount()
	if err != nil {
		return 0, fmt.Errorf("failed to get delta update count: %w", err)
	}
	return count, nil
}

// GetPatchUpdateCount returns the number of patch updates received
func (s *FsdbTestSubscriber) GetPatchUpdateCount() (int64, error) {
	count, err := s.client.GetPatchUpdateCount()
	if err != nil {
		return 0, fmt.Errorf("failed to get patch update count: %w", err)
	}
	return count, nil
}

// GetLastPathUpdateContent returns the content of the last path update
func (s *FsdbTestSubscriber) GetLastPathUpdateContent() (string, error) {
	content, err := s.client.GetLastPathUpdateContent()
	if err != nil {
		return "", fmt.Errorf("failed to get last path update content: %w", err)
	}
	return content, nil
}

// GetLastDeltaUpdateContent returns the content of the last delta update
func (s *FsdbTestSubscriber) GetLastDeltaUpdateContent() (string, error) {
	content, err := s.client.GetLastDeltaUpdateContent()
	if err != nil {
		return "", fmt.Errorf("failed to get last delta update content: %w", err)
	}
	return content, nil
}

// GetLastPatchUpdateContent returns the content of the last patch update
func (s *FsdbTestSubscriber) GetLastPatchUpdateContent() (string, error) {
	content, err := s.client.GetLastPatchUpdateContent()
	if err != nil {
		return "", fmt.Errorf("failed to get last patch update content: %w", err)
	}
	return content, nil
}

// PublishTestState publishes test state data (requires connected publisher)
func (s *FsdbTestSubscriber) PublishTestState(path []string, jsonValue string) error {
	if err := s.client.PublishTestState(path, jsonValue); err != nil {
		return fmt.Errorf("failed to publish test state: %w", err)
	}
	return nil
}

// ConnectPublisher connects the async publisher to FSDB
func (s *FsdbTestSubscriber) ConnectPublisher() error {
	if err := s.client.ConnectPublisher(); err != nil {
		return fmt.Errorf("failed to connect publisher: %w", err)
	}
	return nil
}

// DisconnectPublisher disconnects the async publisher
func (s *FsdbTestSubscriber) DisconnectPublisher() error {
	if err := s.client.DisconnectPublisher(); err != nil {
		return fmt.Errorf("failed to disconnect publisher: %w", err)
	}
	return nil
}

// IsPublisherConnected returns whether the publisher is connected
func (s *FsdbTestSubscriber) IsPublisherConnected() (bool, error) {
	connected, err := s.client.IsPublisherConnected()
	if err != nil {
		return false, fmt.Errorf("failed to check publisher connection: %w", err)
	}
	return connected, nil
}

// WaitForPublisherConnected waits for the publisher to connect
func (s *FsdbTestSubscriber) WaitForPublisherConnected(timeout time.Duration) (bool, error) {
	timeoutMs := int64(timeout / time.Millisecond)
	connected, err := s.client.WaitForPublisherConnected(timeoutMs)
	if err != nil {
		return false, fmt.Errorf("failed to wait for publisher connection: %w", err)
	}
	return connected, nil
}

// WaitForPublisherReady waits for the publisher to be ready (connected and registered on server)
func (s *FsdbTestSubscriber) WaitForPublisherReady(timeout time.Duration) (bool, error) {
	timeoutMs := int64(timeout / time.Millisecond)
	ready, err := s.client.WaitForPublisherReady(timeoutMs)
	if err != nil {
		return false, fmt.Errorf("failed to wait for publisher ready: %w", err)
	}
	return ready, nil
}

// PublishAgentConfigCmdLineArgs publishes typed data to agent.config.defaultCommandLineArgs
func (s *FsdbTestSubscriber) PublishAgentConfigCmdLineArgs(key, value string) error {
	if err := s.client.PublishAgentConfigCmdLineArgs(key, value); err != nil {
		return fmt.Errorf("failed to publish agent config cmd line args: %w", err)
	}
	return nil
}

// PublishAgentSwitchStatePortInfo publishes typed data to agent.switchState.portMaps
func (s *FsdbTestSubscriber) PublishAgentSwitchStatePortInfo(switchID, portID int64, portName string, operState int32) error {
	if err := s.client.PublishAgentSwitchStatePortInfo(switchID, portID, portName, operState); err != nil {
		return fmt.Errorf("failed to publish agent switch state port info: %w", err)
	}
	return nil
}
