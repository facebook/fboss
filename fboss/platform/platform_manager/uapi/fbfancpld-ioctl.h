/* SPDX-License-Identifier: GPL-2.0+ */
/* Copyright (c) Meta Platforms, Inc. and affiliates. */

#ifndef __FBFANCPLD_IOCTL_H__
#define __FBFANCPLD_IOCTL_H__

#include <linux/ioctl.h>
#include <linux/types.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Fan CPLD hardware configuration, passed from userspace via ioctl.
 *
 * All platforms share the same register layout (tach at 0x30+N*0x10,
 * PWM at 0x32+N*0x10, etc.); only these parameters differ.
 */
struct fbfancpld_ioctl_config {
	__u32 num_fans;		/* Number of fan trays (1-8) */
	__u32 pwm_max;		/* Max PWM register value (e.g. 40 or 64) */
	__u32 speed_multiplier;	/* Tach-to-RPM multiplier (e.g. 150 or 300) */
	__u32 has_rear_tach;	/* Non-zero if fans have front+rear tach */
	__u32 has_leds;		/* Non-zero if fan trays have LEDs */
};

#define FBFANCPLD_IOC_MAGIC 'F'
#define FBFANCPLD_IOC_CONFIGURE \
	_IOC(_IOC_WRITE, FBFANCPLD_IOC_MAGIC, 1, \
	     sizeof(struct fbfancpld_ioctl_config))

#ifdef __cplusplus
}
#endif

#endif /* __FBFANCPLD_IOCTL_H__ */
