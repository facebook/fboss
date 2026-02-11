---
sidebar_position: 1
---

# FBOSS CLI

FBOSS provides two CLI binaries for interacting with the switch:

- **fboss2**: Provides the ability to inspect the state of the switch through `show`, `clear`, and operational commands.
- **fboss2-dev**: Includes everything in `fboss2` plus `config` subcommands for managing the switch configuration locally.

## Command Categories

To view all available commands, run `fboss2 -h` or `fboss2-dev -h`.
You can also explore subcommands by running `fboss2 <command> -h` or `fboss2-dev <command> -h`.

### Show Commands

Display current state and statistics:

```bash
fboss2 show interface
fboss2 show port
fboss2 show route
```

Most `show` commands can produce their output in JSON as well with `--fmt json`, which is convenient for scripting.

### Clear Commands

Reset counters and clear state:

```bash
fboss2 clear interface counters
fboss2 clear arp
```

### Config Commands (fboss2-dev only)

Modify the switch configuration:

```bash
fboss2-dev config interface eth1/1/1 mtu 9000
fboss2-dev config qos policy my-policy map tc-to-queue 0 0
```

## Configuration Architecture

### Config Sessions

The `fboss2-dev` CLI uses a **session-based configuration model**. Configuration changes are staged in a session before being applied to the switch.

```
┌─────────────────┐     ┌─────────────────┐     ┌────────────────────┐
│   CLI Command   │────▶│  Config Session │────▶│   Startup Config   │
│                 │     │   (staged)      │     │      (applied)     │
└─────────────────┘     └─────────────────┘     └────────────────────┘
                              │                           │
                              ▼                           ▼
                        ~/.fboss2/agent.conf     /etc/coop/agent.conf
```

**Key points:**

1. **No immediate side effects**: Running a `config` command only updates the session config. The switch state is not modified until the session is committed. You can use `fboss2-dev config session diff` to see the differences between the session config and the startup config.

2. **Atomic commits**: All staged changes are applied together when you run `fboss2-dev config session commit`.

3. **Rebase**: Use `fboss2-dev config session rebase` to reapply uncommitted session changes on top of the current startup config, similar to `git rebase`. This is useful if the startup config has changed since the session was started (e.g., another user committed a change).

4. **Rollback**: Since the configuration is stored in a local Git repository, you can roll back to a previous configuration by checking out a previous commit and reloading the agent.

### Session Workflow

```bash
# Stage configuration changes (no side effects yet)
fboss2-dev config interface eth1/1/1 description "Peer link to device XYZ"
fboss2-dev config interface eth1/1/1 eth1/1/2 mtu 9000

# Review pending changes
fboss2-dev config session diff

# Apply all changes atomically
fboss2-dev config session commit

# Or rebase changes onto current config
fboss2-dev config session rebase
```

### Config Storage

The switch configuration is stored under `/etc/coop/`, which is a **local Git repository**:

- **No remote**: The repository is managed entirely locally with no remote configured.
- **Automatic commits**: When you run `fboss2-dev config session commit`, the CLI automatically creates a Git commit with the new configuration.
- **Version history**: You can use standard Git commands to view configuration history.

```
/etc/coop/
├── .git/              # Local Git repository
├── agent.conf         # Current startup configuration (usually a symlink)
└── cli/
    └── agent.conf     # Last configuration committed by the CLI
```

### Commit Action Levels

When a configuration change is committed, the system determines the appropriate way to apply it depending on what was configured:

| Action Level | Description | Typical Duration |
|--------------|-------------|------------------|
| `HITLESS` | Applied via delta processing without traffic impact | ~1-2 seconds |
| `AGENT_RESTART` | Requires agent restart to apply | ~10+ seconds |

The CLI automatically tracks the required action level based on the types of changes made. Some changes (like QoS map updates) can be applied hitlessly, while others (like port queue configuration) require an agent restart.

### Useful Commands

| Command | Description |
|---------|-------------|
| `show running-config` | Show the current in-memory configuration of the agent |
| `config session diff` | Show pending changes between session and startup config |
| `config session commit` | Apply staged changes and reload the agent |
| `config session rebase` | Reapply uncommitted changes on top of current startup config |
| `config history` | Show Git commit history of configuration changes |
| `config rollback` | Roll back to a previous configuration version |
| `config reload` | Reload the agent configuration from the startup config |

**Note:** The running config (in-memory) may differ from the startup config (`/etc/coop/agent.conf`) if changes have been made directly via Thrift calls to the agent.

## See Also

- [FBOSS Software Architecture](/docs/architecture/fboss_sw_architecture.md)
