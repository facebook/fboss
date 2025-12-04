/* SPDX-License-Identifier: GPL-2.0+ */
/* Copyright (c) Meta Platforms, Inc. and affiliates. */

#ifndef __LINUX_FBIOB_IOCTL_H__
#define __LINUX_FBIOB_IOCTL_H__

#include <linux/ioctl.h>
#include <linux/limits.h>
#include <linux/types.h>

#ifdef __cplusplus
extern "C" {
#endif

#define FBIOB_SPIDEV_MAX	1
#define FBIOB_INVALID_OFFSET	((__u32)-1)

/*
 * I2C Controller settings.
 *
 * Both "bus_freq_hz" and "num_channels" are optional: set them to 0 if
 * they are not applicable to the I2C controller.
 */
struct fbiob_i2c_data {
	__u32 bus_freq_hz;
	__u32 num_channels;
};

/*
 * "spi_dev_info" is used to build the "spi_board_info" structure.
 * NOTE: "spi_board_info" is not accessible from user space.
 */
struct spi_dev_info {
	char modalias[NAME_MAX];
	__u32 max_speed_hz;
	__u16 chip_select;
};

/*
 * SPI bus settings.
 *
 * This argument MUST be provided when initializing a SPI bus.
 */
struct fbiob_spi_data {
	unsigned int num_spidevs;
	struct spi_dev_info spidevs[FBIOB_SPIDEV_MAX];
};

/*
 * LED specific settings:
 *   - "port_num" is only applicable to port LEDs.
 *      Set the value to -1 for other LED types.
 *   - "led_idx" is to support multiple LED instances per port/type.
 *     Set the value to -1 if not applicable.
 *   - Both "port_num" and "led_idx" must be 1-based.
 */
struct fbiob_led_data {
	int port_num;
	int led_idx;
};

/*
 * XCVR specific settings.
 *   - This is only consumed by "xcvr_ctrl" driver, which exports sysfs
 *     entries for XCVR reset, low_power, etc.
 *   - "port_num" must be 1-based.
 */
struct fbiob_xcvr_data {
	__u32 port_num;
};

/*
 * Fan/PWM controller data.
 */
struct fbiob_fan_data {
	__u32 num_fans;
};

/*
 * The structure to uniquely identify an I/O controller within the FPGA.
 *   - <name> is used to match the controller driver.
 *   - <id> is the controller instance ID within the FPGA.
 */
struct fbiob_aux_id {
	char name[NAME_MAX];
	__u32 id;
};

/*
 * The structure contains all the required information to initialize an
 * I/O controller within the FPGA.
 */
struct fbiob_aux_data {
	struct fbiob_aux_id id;

	__u32 csr_offset;
	__u32 iobuf_offset;

	/*
	 * It's callers' responsiblity to fill the below union structure
	 * properly.
	 * The structure is "transparent" to the FPGA PCIe driver: it is
	 * parsed in the I/O controller driver.
	 */
	union {
		struct fbiob_fan_data fan_data;
		struct fbiob_i2c_data i2c_data;
		struct fbiob_led_data led_data;
		struct fbiob_spi_data spi_data;
		struct fbiob_xcvr_data xcvr_data;
	};
};

/*
 * The structure contains all the required information to initiate MDIO
 * C45 transactions from user space.
 */
struct fb_mdio_xfer {
	__u32 phy_addr;	/* 5-bit PHY address. */
	__u32 dev_addr;	/* 5-bit device address. */
	__u32 reg_addr;	/* 16-bit register address. */
	__u32 reg_data;	/* 16-bit register value. */
};

/*
 * ioctl commands for auxdev creation/deletion.
 */
#define FBIOB_IOC_MAGIC		'F'
#define FBIOB_IOC_NEW_DEVICE	_IOW(FBIOB_IOC_MAGIC, 1, struct fbiob_aux_data)
#define FBIOB_IOC_DEL_DEVICE	_IOW(FBIOB_IOC_MAGIC, 2, struct fbiob_aux_data)

/*
 * ioctl commands for MDIO transactions.
 * NOTE: each C45 transfer takes 2 steps (address phase and data phase),
 * and FPGA/BSP will combine the 2 steps into an "atomic" operation.
 */
#define FBIOC_MDIO_C45_WRITE	_IOW(FBIOB_IOC_MAGIC, 1, struct fb_mdio_xfer)
#define FBIOC_MDIO_C45_READ	_IOW(FBIOB_IOC_MAGIC, 2, struct fb_mdio_xfer)

#ifdef __cplusplus
}
#endif

#endif /* __LINUX_FBIOB_IOCTL_H__ */
