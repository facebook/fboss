# Conveyor Analyzer

A Python CLI tool for analyzing and managing conveyor release instances. This tool helps you quickly identify failed nodes, track node history across releases, and retry failed builds.

## Features

- 📊 **Release Analysis**: View detailed status of any release instance
- 📋 **Multi-Release Overview**: Compare status across multiple releases
- 🔍 **Node History**: Track a specific node's status across release instances
- 🔄 **Retry Failed Nodes**: Interactively or automatically retry failed/cancelled nodes
- 📈 **Distance Metrics**: Calculate how far a release is from qualification
- 🎨 **Rich Formatting**: Colored output with clickable links to Sandcastle jobs and console logs

## Installation

Build the tool using Buck2:

```bash
buck2 build //fboss/scripts/conveyor_analyzer:fboss_conveyor
```

## Usage

The tool provides a single `analyze` command with multiple modes of operation:

### 1. Analyze a Specific Release

View detailed status of a single release instance:

```bash
fboss_conveyor analyze --conveyor fboss/wedge_agent --release R6613.3
```

**Output:**
- Release summary with pass/fail/cancelled/not-run counts
- Distance from qualification metric
- Table of failed nodes with links to Sandcastle jobs and console logs
- Error categorization (infra vs app errors)

### 2. Multi-Release Overview

Compare status across multiple recent releases:

```bash
fboss_conveyor analyze --conveyor fboss/wedge_agent --limit 10
```

**Output:**
- Table showing the last 10 releases
- Status counts for each release
- Distance from qualification with visual indicators (✅ ⚠️)

### 3. Node History Across Releases

Track a specific node's behavior across all instances of a release:

```bash
fboss_conveyor analyze --conveyor fboss/wedge_agent --release R6613 --node my_test_node
```

**Note:** When tracking node history, specify the release number without the instance (e.g., `R6613`, not `R6613.3`).

**Output:**
- Table showing the node's status across all release instances
- Duration of each run
- Links to Sandcastle jobs and console logs
- Error messages for failed runs

### 4. Retry Failed Nodes

Interactively select and retry failed or cancelled nodes:

```bash
fboss_conveyor analyze --conveyor fboss/wedge_agent --release R6613.3 --retry-failed
```

This will:
1. List all failed/cancelled nodes
2. Prompt you to select which nodes to retry (enter numbers, comma-separated, or "all")
3. Execute the retry command for selected nodes

To retry all failed nodes without prompting:

```bash
fboss_conveyor analyze --conveyor fboss/wedge_agent --release R6613.3 --retry-failed --all
```

## Command Options

| Option | Description | Required |
|--------|-------------|----------|
| `--conveyor` | Conveyor ID (e.g., `fboss/wedge_agent`) | Yes |
| `--release` | Release instance (e.g., `R6613.3` or `R6613` for node history) | No |
| `--node` | Specific node name to analyze | No |
| `--limit` | Number of releases to show in multi-release view (default: 10) | No |
| `--retry-failed` | Enable retry mode for failed/cancelled nodes | No |
| `--all` | Retry all failed nodes without prompting (use with `--retry-failed`) | No |

## Understanding the Output

### Status Counts

- **✅ Passed**: Nodes that completed successfully
- **❌ Failed**: Nodes that failed execution
- **⏸️ Cancelled**: Nodes that were cancelled
- **⏳ Ongoing**: Nodes currently running
- **⏭️ Not Run**: Nodes that haven't been scheduled yet

### Distance from Qualification

The distance metric shows how many issues prevent a release from qualifying:

```
Distance = FAILED + CANCELLED + NOT_RUN
```

Visual indicators:
- ✅ `0 away from qual` - Release is qualified
- ⚠️ `1-3 away from qual` - Close to qualification
- `>3 away from qual` - Multiple issues to resolve

### Error Categories

Failed nodes are categorized to help identify root causes:

- **infra**: Infrastructure issues (timeouts, capacity, network)
- **app**: Application/test failures (assertions, test failures)
- **unknown**: Uncategorized errors

## Examples

### Quick health check of latest releases
```bash
fboss_conveyor analyze --conveyor fboss/wedge_agent --limit 5
```

### Debug a specific failing node
```bash
fboss_conveyor analyze --conveyor fboss/wedge_agent --release R6613 --node flaky_test_node
```

### Fix a release by retrying failures
```bash
fboss_conveyor analyze --conveyor fboss/wedge_agent --release R6613.3 --retry-failed
```

### Auto-retry all failures
```bash
fboss_conveyor analyze --conveyor fboss/wedge_agent --release R6613.3 --retry-failed --all
```

## Architecture

The tool is organized into several modules:

- **`cli.py`**: Command-line interface using Click
- **`conveyor_api.py`**: Wrapper for conveyor CLI commands
- **`parser.py`**: Data models and JSON parsing logic
- **`formatter.py`**: Output formatting with rich tables
- **`utils.py`**: Utility functions for parsing and analysis
- **`fboss_conveyor.py`**: Main entry point

### Data Model

```python
@dataclass
class NodeRun:
    node_name: str
    status: Optional[str]  # SUCCESSFUL, FAILED, CANCELLED, ONGOING, PAUSED, or None
    scheduled_start_time: Optional[datetime]
    actual_start_time: Optional[datetime]
    actual_end_time: Optional[datetime]
    run_details: str
    attempt: int
    sandcastle_job_link: Optional[str]
    console_log_link: Optional[str]
    error_message: Optional[str]

@dataclass
class Release:
    release_number: int
    release_instance_number: int
    release_instance_id: str
    runs: Dict[str, NodeRun]
    creation_time: Optional[datetime]
    creator: Optional[str]
```

## Testing

Run the test suite:

```bash
buck2 test //fboss/scripts/conveyor_analyzer:test_parser
buck2 test //fboss/scripts/conveyor_analyzer:test_utils
```

## Development

### Adding New Features

1. Add logic to appropriate module (`parser.py`, `formatter.py`, `utils.py`)
2. Add tests in the `tests/` directory
3. Update CLI command in `cli.py` if needed
4. Run `arc lint -a` to auto-format and update dependencies

### Code Style

The project follows Meta's Python standards:
- BLACK code formatting (enforced by `arc lint`)
- Type hints on all functions
- Docstrings for public APIs
- Single-responsibility functions under 50 lines

## Troubleshooting

### "Conveyor CLI failed" error

Make sure you have the `conveyor` CLI tool installed and configured:
```bash
which conveyor
```

### "No releases found" error

Verify the conveyor ID is correct:
```bash
conveyor release status --conveyor-id fboss/wedge_agent --limit 1 --json
```

### Links not clickable

Some terminals don't support ANSI hyperlinks. The raw URLs are still displayed for copy-pasting.

## Contributing

1. Make your changes
2. Add tests for new functionality
3. Run tests: `buck2 test //fboss/scripts/conveyor_analyzer:...`
4. Format code: `arc lint -a`
5. Submit your diff

## License

Copyright (c) Meta Platforms, Inc. and affiliates.
