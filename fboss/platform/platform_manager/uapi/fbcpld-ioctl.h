/* SPDX-License-Identifier: GPL-2.0+ */
/* Copyright (c) Meta Platforms, Inc. and affiliates. */

#ifndef __FBCPLD_IOCTL_H__
#define __FBCPLD_IOCTL_H__

#include <linux/ioctl.h>
#include <linux/limits.h>
#include <linux/types.h>

#ifdef __cplusplus
extern "C" {
#endif

#define FBCPLD_MAX_ATTRS 64
#define FBCPLD_HELP_STR_MAX 256

/*
 * Flags for sysfs attribute behavior (mirrors RBS_FLAG_* from regbit-sysfs.h)
 */
#define FBCPLD_FLAG_LOG_WRITE (1U << 0)
#define FBCPLD_FLAG_SHOW_NOTES (1U << 1)
#define FBCPLD_FLAG_SHOW_DEC (1U << 2)
#define FBCPLD_FLAG_VAL_NEGATE (1U << 3)

/*
 * Structure describing a single sysfs attribute to create.
 */
struct fbcpld_sysfs_attr {
  char name[NAME_MAX];                /* Sysfs file name */
  __u32 mode;                         /* 0444 (RO), 0644 (RW), or 0200 (WO) */
  __u32 reg_addr;                     /* Register address (0-255) */
  __u32 bit_offset;                   /* Bit position in register */
  __u32 num_bits;                     /* Number of bits */
  __u32 flags;                        /* FBCPLD_FLAG_* flags */
  char help_str[FBCPLD_HELP_STR_MAX]; /* Optional help text */
};

/*
 * ioctl request to create sysfs attributes.
 */
struct fbcpld_ioctl_create_request {
  __u32 num_attrs;
  struct fbcpld_sysfs_attr attrs[FBCPLD_MAX_ATTRS];
};

/*
 * ioctl commands for dynamic sysfs management.
 */
#define FBCPLD_IOC_MAGIC 'C'
#define FBCPLD_IOC_SYSFS_CREATE _IOC(_IOC_WRITE, FBCPLD_IOC_MAGIC, 1, 0)
#define FBCPLD_IOC_SYSFS_DESTROY_ALL _IO(FBCPLD_IOC_MAGIC, 3)

#ifdef __cplusplus
}
#endif

#endif /* __FBCPLD_IOCTL_H__ */
