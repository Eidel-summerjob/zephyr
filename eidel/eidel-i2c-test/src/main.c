/*
 * Copyright (c) 2012-2014 Wind River Systems, Inc.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#include <errno.h>
#include <zephyr.h>
#include <sys/printk.h>

#include <device.h>
#include <drivers/i2c.h>

//#define I2C_NAME DT_LABEL(DT_ALIAS(i2c0))
#define I2C_NAME DT_LABEL(DT_NODELABEL(i2c0))
#define I2C_NMSGS 5
#define I2C_MSGS_LEN 20
#define I2C_REMOTE_ADDR 0x55

void main(void)
{
	printk("i2c name = %s\n", I2C_NAME);
	const struct device *dev = device_get_binding(I2C_NAME);
	if (!dev)
	{
		printk("Error: device_get_binding: %i\n", errno);
		return;
	}

	struct i2c_msg msgs[I2C_NMSGS];
	for (int i = 0; i < I2C_NMSGS; i++)
	{
		msgs->len = I2C_MSGS_LEN;
		msgs->flags = I2C_MSG_WRITE;
		for (int j = 0; j < I2C_MSGS_LEN; j++)
			msgs->buf[i] = 0xAA;
	}

	int ret = i2c_transfer(dev, msgs, I2C_NMSGS, I2C_REMOTE_ADDR);
	if (ret)
	{
		printk("Error: write_bytes: %i\n", errno);
	}
	else
	{
		printk("Success!");
	}
}
