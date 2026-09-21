---
id: running
title: Running PlatformManager
sidebar_label: Running
sidebar_position: 7
---

# Running PlatformManager

PlatformManager can run in two modes: one-shot mode for initial platform setup,
or daemon mode for continuous operation with a Thrift service.

## One-Shot Mode (Default)

```bash
platform_manager --run_once=true
```
1. Loads the platform configuration
2. Explores the platform and creates device symlinks
3. Exits with status 0 on success, 1 on failure


## Daemon Mode

```bash
platform_manager --run_once=false
```
Performs the same steps as one-shot mode, but instead of exiting:
1. Starts the Thrift service on the configured port
2. Continues running to handle API requests
3. Sends `sd_notify` ready signal for systemd integration



<br/>
<br/>
<br/>


:::info
For a complete list of available arguments, run `platform_manager --help`.
:::
