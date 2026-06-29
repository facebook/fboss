# Supported and Planned Kernel Versions

This page is the single source of truth for the Linux kernel versions that FBOSS
BSP kernel modules must support. This page should be used to plan kmod
builds and qualification against current and upcoming kernels.

## Supported Kernel Versions

BSP kernel modules must be compatible with the following kernel versions
(updated in May 2026):

| Kernel Version | Status    |
|----------------|-----------|
| Linux v6.4.3   | Supported |
| Linux v6.11.1  | Supported |
| Linux v6.16.1  | Supported |

## Planned Kernel Versions

The following kernel versions are planned for adoption. Exact patch version and
timelines are subject to change; vendors should begin planning kmod builds and
qualification against these versions ahead of the migration.

| Kernel Version | Status  |
|----------------|---------|
| Linux v7.1     | Planned |

## What "Compatible" Means

Being compatible with a kernel version means:

1. The kernel modules can be compiled against the kernel version successfully.
2. The kernel modules can be loaded in the kernel version and behave as
   expected.

## Testing Requirements

The per-driver test requirements that kmods must pass on each supported kernel
version are defined in the
[FBOSS BSP Kernel Module Development Requirements](./bsp_development_requirements.md)
document (see the "Test Requirement(s)" subsections under each driver type).
