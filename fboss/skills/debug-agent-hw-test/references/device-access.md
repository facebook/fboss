# Device Access (Meta External)

How to execute commands, upload files, and download files on the test switch.

## Prerequisites

You need SSH access to your test switch. Verify with:

```bash
ssh <user>@<switch> "hostname"
```

> **Customization point**: If your environment uses a different device
> access method (e.g., an MCP server, console server, or lab management
> API), create `facebook/device-access.md` in this skill directory with
> your method. The skill will automatically prefer it over this file.

## Commands

### Run a command on the switch

```bash
ssh <user>@<switch> "<command>"
```

### Run a long-running command in the background

```bash
ssh <user>@<switch> "nohup <command> > /tmp/output.log 2>&1 &"

# Check if still running:
ssh <user>@<switch> "pgrep -f '<process_name>' && echo RUNNING || echo DONE"

# Get output:
ssh <user>@<switch> "tail -50 /tmp/output.log"
```

### Upload a file to the switch

```bash
scp <local_path> <user>@<switch>:<remote_path>
```

### Download a file from the switch

```bash
scp <user>@<switch>:<remote_path> <local_path>
```
