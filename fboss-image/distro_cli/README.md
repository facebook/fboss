# FBOSS Image Builder CLI

Manifest-driven tool for building FBOSS distribution images and managing devices.

## Quick Start

Requires Python 3.10+. No external dependencies.

```bash
# Build an image
./fboss-image build manifest.json

# Build specific components
./fboss-image build manifest.json kernel sai

# Device management
./fboss-image device <mac-address> image-upstream kernel sai
./fboss-image device <mac-address> image image.bin
./fboss-image device <mac-address> reprovision
./fboss-image device <mac-address> update manifest.json kernel
./fboss-image device <mac-address> getip
./fboss-image device <mac-address> ssh
```

## Manifest Format

The manifest is a JSON file that defines what components to build, where to get them, and how to compose the final FBOSS distribution image.

### What Goes Into the Manifest

The manifest describes your FBOSS image composition through these sections:

**1. Output Formats** (`distribution_formats` - required)

Specifies what image formats to produce. Supported formats:
- `onie`: ONIE installer binary for network switches
- `usb`: Bootable ISO image for USB installation

**2. Kernel** (`kernel` - required)

Where to get the Linux kernel. Specify a `download` URL pointing to the kernel tarball.

**3. Dependencies** (`other_dependencies` - optional)

Additional packages needed for your image (RPMs, tools, etc.). Each dependency specifies:
- `download`: URL (http/https) or local file path (`file:path/to/package.rpm`)

**4. FBOSS Components** (all optional)

The FBOSS software stack components to build:
- `fboss-platform-stack`: Platform services and hardware abstraction
- `bsps`: Board Support Packages for specific hardware platforms (array)
- `sai`: Switch Abstraction Interface implementation
- `fboss-forwarding-stack`: Core switching/routing logic

Each component specifies:
- `execute`: Command to build it (string or array of command + args)

**5. Build Hooks** (`image_build_hooks` - optional)

Customization points in the build process:
- `after_pkgs`: Additional package configuration for extra OS packages

### Example

```json
{
  "distribution_formats": {
    "onie": "FBOSS-k6.4.3.bin",
    "usb": "FBOSS-k6.4.3.iso"
  },
  "kernel": {
    "download": "https://artifact..../fboss/fboss-oss_kernel_v6.4.3.tar"
  },
  "other_dependencies": [
    {"download": "https://artifact.github.com/.../fsdb/fsdb.rpm"},
    {"download": "file:vendor_debug_tools/tools.rpm"}
  ],
  "fboss-platform-stack": {
    "execute": ["fboss/fboss/oss/scripts/build.py", "platformstack"]
  },
  "bsps": [
    {"execute": "vendor_bsp/build.make"},
    {"execute": "fboss/oss/reference_bsp/build.py"}
  ],
  "sai": {
    "execute": "fboss_brcm_sai/build.sh"
  },
  "fboss-forwarding-stack": {
    "execute": ["fboss/fboss/oss/scripts/build.py", "forwardingstack"]
  },
  "image_build_hooks": {
    "after_pkgs": "additional_os_pkgs.xml"
  }
}
```

## Build Order

Components are built in the following order:
1. `kernel`
2. `other_dependencies`
3. `fboss-platform-stack`
4. `bsps`
5. `sai`
6. `fboss-forwarding-stack`
7. Image composition with `distribution_formats`

## Development

### Running Tests

```bash
# Run all tests
cd fboss-image/distro_cli
python3 -m unittest discover -s tests -p '*_test.py'

# Run specific test
python3 -m unittest tests.cli_test
```

### Linting

```bash
cd fboss-image/distro_cli
python3 -m ruff check .
```

### Project Structure

```
fboss-image/distro_cli/
├── fboss-image          # Main CLI entry point
├── lib/                 # Core libraries
│   ├── cli.py          # CLI framework (argparse abstraction)
│   ├── manifest.py     # Manifest parsing and validation
│   ├── builder.py      # Image builder
│   └── logger.py       # Logging setup
├── cmds/               # Command implementations
│   ├── build.py        # Build command
│   └── device.py       # Device commands
└── tests/              # Unit tests
    ├── cli_test.py     # CLI framework tests
    ├── manifest_test.py
    ├── image_builder_test.py
    ├── build_test.py
    └── device_test.py
```

### CLI Framework

The CLI uses a custom OOP wrapper around argparse (stdlib only, no external dependencies):

```python
from lib.cli import CLI

# Create CLI
cli = CLI(description='My CLI')

# Add simple command
cli.add_command('build', build_func,
                help_text='Build something',
                arguments=[('file', {'help': 'Input file'})])

# Add command group with subcommands
device = cli.add_command_group('device',
                               help_text='Device commands',
                               arguments=[('mac', {'help': 'MAC address'})])
device.add_command('ssh', ssh_func, help_text='SSH to device')

# Run
cli.run(setup_logging_func=setup_logging)
```
