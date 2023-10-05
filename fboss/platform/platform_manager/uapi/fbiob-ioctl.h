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
 * I2C bus timing settings.
 *
 * It's an optional argument:
 *   1. If the I2C controller driver supports the argument:
 *      - if <bus_freq_hz> is valid, the timing settings will be applied
 *        at controller setup time.
 *      - if <bus_freq_hz> is invalid (out of range), the value will be
 *        ignored and the default timing settings is applied.
 *
 *   2. If the I2C controller driver doesn't support customized timing,
 *      <bus_freq_hz> is simply ignored.
 */
struct fbiob_i2c_data {
	__u32 bus_freq_hz;
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
		struct fbiob_spi_data spi_data;
		struct fbiob_i2c_data i2c_data;
	};
};

/*
 * Supported ioctl commands.
 */
#define FBIOB_IOC_MAGIC		'F'
#define FBIOB_IOC_NEW_DEVICE	_IOW(FBIOB_IOC_MAGIC, 1, struct fbiob_aux_data)
#define FBIOB_IOC_DEL_DEVICE	_IOW(FBIOB_IOC_MAGIC, 2, struct fbiob_aux_data)

#ifdef __cplusplus
}
#endif

#endif /* __LINUX_FBIOB_IOCTL_H__ */
