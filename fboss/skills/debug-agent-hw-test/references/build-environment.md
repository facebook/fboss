# Build Environment (Meta External)

How to build FBOSS test binaries, locate config files, and find vendor SDK artifacts.

> **Customization point**: If your environment has a different build system
> or SDK layout, create `facebook/build-environment.md` in this skill
> directory with your environment-specific commands and paths. The skill
> will automatically prefer it over this file.

## Building Test Binaries

FBOSS uses CMake for open-source builds. See `fboss/oss/scripts/` for
build scripts and `fboss/oss/README.md` for setup instructions.

### Mono (sai_agent_hw_test)

```bash
cd fboss
./oss/scripts/run-getdeps.py --install-dir /opt/fboss \
  --sai-impl <vendor-sai> \
  --target sai_agent_hw_test
```

### Multi-switch (multi_switch_agent_hw_test + fboss_hw_agent)

Multi-switch mode requires **two** binaries:

```bash
./oss/scripts/run-getdeps.py --install-dir /opt/fboss \
  --sai-impl <vendor-sai> \
  --target multi_switch_agent_hw_test

./oss/scripts/run-getdeps.py --install-dir /opt/fboss \
  --sai-impl <vendor-sai> \
  --target fboss_hw_agent
```

### Build Output Paths

Built binaries are placed under the install directory:

```
/opt/fboss/bin/sai_agent_hw_test
/opt/fboss/bin/multi_switch_agent_hw_test
/opt/fboss/bin/fboss_hw_agent
```

Adjust the install directory (`--install-dir`) as needed for your environment.

## Config Files

FBOSS ships example agent configs under:

```
fboss/agent/test/agent_hw_test_configs/
```

Copy and customize for your platform. See
[build-and-load.md](build-and-load.md#config-files) for the naming
convention.

## Broadcom DNX Firmware Location

Broadcom DNX platforms require the firmware `db/` directory. Locate it
in your Broadcom SDK installation, typically under:

```
<your-bcm-sdk-path>/tools/sand/db
```

The exact path depends on your SDK version and installation layout.
Consult your Broadcom SDK release notes or installation guide.

## Leaba/Cisco SDK Libraries Location

Leaba ASIC targets require `res/` and `lib/` from the Leaba SDK.
Locate them in your SDK installation, typically:

```
<your-leaba-sdk-path>/res
<your-leaba-sdk-path>/lib
```

These may be symlinks — use `tar -h` (follow symlinks) when creating
tarballs to ensure real files are included.
