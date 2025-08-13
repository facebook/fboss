---
id: troubleshooting
title: Troubleshooting
description: Solutions to common problems vendors may face
keywords:
    - FBOSS
    - OSS
    - onboard
    - troubleshoot
    - faq
oncall: fboss_oss
---

## Binaries Will Not Start

If a binary fails to run due to a missing dependency, ensure that environment
variables are correctly set:
```bash file=./static/code_snips/check_dependencies.sh
```

## setup.py Fails

We use `setup.py` to populate `fruid.json` and other config files, but it may
not succeed for all platforms. In this case, you can source this script to set
environment variables:

```bash
cd /opt/fboss
source ./bin/setup_fboss_env
```

## Building FBOSS Docker Image Fails

Instead of building the Docker image yourself, you can obtain an image tarball
from GitHub. The published image includes all relevant sources, including
sources for dependencies, which means that building FBOSS binaries from this
image eliminates the need for a network connection at the time of building. To
obtain the image, follow this guide:

```bash file=./static/code_snips/load_docker_image.sh
```

## Build Doesn't Pick Up Local Changes

If you want to make changes locally and then build, you will need another flag
which tells `getdeps.py` to pick them up. You can use the flag `--src-dir` to
tell it where changes are located:

```bash file=./static/code_snips/build_with_src_dir.sh
```
