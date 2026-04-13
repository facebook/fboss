// (c) Meta Platforms, Inc. and affiliates. Confidential and proprietary.

package main

import (
	"flag"
	"fmt"
	"os"
	"os/signal"
	"strings"
	"syscall"
	"time"

	"fboss/fsdb/client/test/fsdb_cgo_client"
)

var (
	// Flags
	stats                  = flag.Bool("stats", false, "Subscribe to stats instead of state")
	host                   = flag.String("host", "::1", "FSDB server host")
	port                   = flag.Int("port", 5908, "FSDB server port")
	consumeDelayMs         = flag.Int("consumeDelayMs", 0, "Delay in milliseconds before consuming each chunk")
	resetDelayAfterNChunks = flag.Int("resetDelayAfterNChunks", 0, "Reset delay after N chunks")
	printChunks            = flag.Bool("printChunks", false, "Print received chunks to stdout")
	count                  = flag.Bool("count", false, "Print count of received chunks")
)

func usage() {
	fmt.Fprintf(os.Stderr, "Usage: %s <streamType> <path> [flags]\n", os.Args[0])
	fmt.Fprintf(os.Stderr, "\nStream Types:\n")
	fmt.Fprintf(os.Stderr, "  subscribePath  - Subscribe to path updates\n")
	fmt.Fprintf(os.Stderr, "  subscribeDelta - Subscribe to delta updates\n")
	fmt.Fprintf(os.Stderr, "  subscribePatch - Subscribe to patch updates\n")
	fmt.Fprintf(os.Stderr, "\nPath Format:\n")
	fmt.Fprintf(os.Stderr, "  '#'- or '/'-separated path tokens (e.g., agent/switchState/portMaps)\n")
	fmt.Fprintf(os.Stderr, "  For subscribePatch, a comma-separated list of paths is supported\n")
	fmt.Fprintf(os.Stderr, "  (e.g., agent/switchState/portMaps,agent/switchState/switchSettingsMap)\n")
	fmt.Fprintf(os.Stderr, "\nFlags may appear before or after positional arguments.\n")
	fmt.Fprintf(os.Stderr, "\nFlags:\n")
	flag.PrintDefaults()
	fmt.Fprintf(os.Stderr, "\nExample:\n")
	fmt.Fprintf(os.Stderr, "  %s subscribePath agent#config --stats --printChunks\n", os.Args[0])
}

// splitArgs separates flag arguments from positional arguments, allowing flags
// to appear in any position (before or after positional args). It correctly
// handles both "-flag value" and "-flag=value" forms, and bool flags that
// require no value argument.
func splitArgs(args []string) (flagArgs, positional []string) {
	i := 0
	for i < len(args) {
		arg := args[i]
		if arg == "--" {
			positional = append(positional, args[i+1:]...)
			return
		}
		if !strings.HasPrefix(arg, "-") {
			positional = append(positional, arg)
			i++
			continue
		}
		// Flag arg: strip leading dashes to get the flag name.
		name := strings.TrimLeft(arg, "-")
		if strings.ContainsRune(name, '=') {
			// "-flag=value" form — self-contained.
			flagArgs = append(flagArgs, arg)
			i++
			continue
		}
		// "-flag" form: check whether this flag accepts a value.
		f := flag.CommandLine.Lookup(name)
		isBool := false
		if f != nil {
			if bf, ok := f.Value.(interface{ IsBoolFlag() bool }); ok {
				isBool = bf.IsBoolFlag()
			}
		}
		if !isBool && i+1 < len(args) && !strings.HasPrefix(args[i+1], "-") {
			// Non-bool flag: consume the next arg as its value.
			flagArgs = append(flagArgs, arg, args[i+1])
			i += 2
		} else {
			flagArgs = append(flagArgs, arg)
			i++
		}
	}
	return
}

func main() {
	flag.Usage = usage

	// Parse flags regardless of their position relative to positional args.
	flagArgs, positional := splitArgs(os.Args[1:])
	if err := flag.CommandLine.Parse(flagArgs); err != nil {
		fmt.Fprintf(os.Stderr, "Error parsing flags: %v\n", err)
		os.Exit(1)
	}

	// Check required arguments
	if len(positional) < 2 {
		fmt.Fprintf(os.Stderr, "Error: streamType and path are required\n\n")
		usage()
		os.Exit(1)
	}

	streamType := positional[0]
	pathStr := positional[1]

	// parseSinglePath splits one path string into tokens, accepting '/' or '#'.
	parseSinglePath := func(s string) []string {
		if strings.ContainsRune(s, '/') && !strings.ContainsRune(s, '#') {
			return strings.Split(s, "/")
		}
		return strings.Split(s, "#")
	}
	// For subscribePatch a comma-separated list of paths may be provided.
	// For other stream types only the first path is used.
	pathStrs := strings.Split(pathStr, ",")
	var paths [][]string
	for _, p := range pathStrs {
		paths = append(paths, parseSinglePath(strings.TrimSpace(p)))
	}
	path := paths[0]

	// Create subscriber
	subscriber := fsdb_cgo_client.NewFsdbSubscriber("cgo_client", *host, int32(*port))
	defer func() {
		if err := subscriber.Cleanup(); err != nil {
			fmt.Fprintf(os.Stderr, "Error during cleanup: %v\n", err)
		}
	}()

	// Initialize subscriber
	if err := subscriber.Init(); err != nil {
		fmt.Fprintf(os.Stderr, "Failed to initialize subscriber: %v\n", err)
		os.Exit(1)
	}

	// Setup signal handling for graceful shutdown
	sigChan := make(chan os.Signal, 1)
	signal.Notify(sigChan, os.Interrupt, syscall.SIGTERM)

	// Subscribe based on stream type
	var err error
	switch streamType {
	case "subscribePath":
		if *stats {
			err = subscriber.SubscribeStatPath(path)
		} else {
			err = subscriber.SubscribeStatePath(path)
		}
		if err != nil {
			fmt.Fprintf(os.Stderr, "Failed to subscribe to path: %v\n", err)
			os.Exit(1)
		}
		fmt.Printf("Subscribed via FsdbPubSubManager (%s path). Press Ctrl+C to exit.\n",
			map[bool]string{true: "stat", false: "state"}[*stats])

	case "subscribeDelta":
		if *stats {
			err = subscriber.SubscribeStatDelta(path)
		} else {
			err = subscriber.SubscribeStateDelta(path)
		}
		if err != nil {
			fmt.Fprintf(os.Stderr, "Failed to subscribe to delta: %v\n", err)
			os.Exit(1)
		}
		fmt.Printf("Subscribed via FsdbPubSubManager (%s delta). Press Ctrl+C to exit.\n",
			map[bool]string{true: "stat", false: "state"}[*stats])

	case "subscribePatch":
		err = subscriber.SubscribeStatePatch(paths, *stats)
		if err != nil {
			fmt.Fprintf(os.Stderr, "Failed to subscribe to patch: %v\n", err)
			os.Exit(1)
		}
		fmt.Printf("Subscribed via FsdbSubManager (%s patch). Press Ctrl+C to exit.\n",
			map[bool]string{true: "stat", false: "state"}[*stats])

	default:
		fmt.Fprintf(os.Stderr, "Error: Invalid streamType '%s'\n", streamType)
		fmt.Fprintf(os.Stderr, "Choose from: subscribePath, subscribeDelta, or subscribePatch\n")
		os.Exit(1)
	}

	// Polling loop
	nChunksConsumed := 0
	done := false

	go func() {
		<-sigChan
		fmt.Println("\nReceived interrupt signal, shutting down...")
		done = true
	}()

	for !done {
		// Wait for update with a reasonable timeout to check for signals
		timeout := 1 * time.Second
		received, err := subscriber.WaitForUpdate(timeout)
		if err != nil {
			fmt.Fprintf(os.Stderr, "Error waiting for update: %v\n", err)
			break
		}

		if !received {
			// Timeout, check if we should continue
			continue
		}

		nChunksConsumed++

		// Print count if requested
		if *count {
			fmt.Printf("Received chunk %d\n", nChunksConsumed)
		}

		// Print content if requested
		if *printChunks {
			var content string
			switch streamType {
			case "subscribePath":
				content, err = subscriber.GetLastPathUpdateContent()
				if err != nil {
					fmt.Fprintf(os.Stderr, "Error getting path content: %v\n", err)
					continue
				}
				fmt.Printf("New value: %s\n", content)

			case "subscribeDelta":
				content, err = subscriber.GetLastDeltaUpdateContent()
				if err != nil {
					fmt.Fprintf(os.Stderr, "Error getting delta content: %v\n", err)
					continue
				}
				fmt.Printf("New Delta: %s\n", content)

			case "subscribePatch":
				content, err = subscriber.GetLastPatchUpdateContent()
				if err != nil {
					fmt.Fprintf(os.Stderr, "Error getting patch content: %v\n", err)
					continue
				}
				fmt.Printf("Received patch update: %s\n", content)
			}
		}

		// Apply consume delay if configured
		if *consumeDelayMs > 0 && (*resetDelayAfterNChunks == 0 || nChunksConsumed <= *resetDelayAfterNChunks) {
			time.Sleep(time.Duration(*consumeDelayMs) * time.Millisecond)
		}
	}

	fmt.Println("Shutting down gracefully...")
}
