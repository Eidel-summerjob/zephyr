/*
 * Copyright (c) 2021 Eidel AS
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DR_DRV_COMPAT gr716a_i2c

#include <drv/i2cmst.h>
#include <drivers/i2c.h>

#include <errno.h>

#define DO_DEBUG 1
#if DO_DEBUG
#define DEBUG(str) str
#else
#define DEBUG(str)
#endif

///////////////////////////////////////////////////////////////////////////////
// Configuration
///////////////////////////////////////////////////////////////////////////////

struct i2cmst_gr716a_config {
    int retries;
    int interrupt_enable;
    int dev_no;
};

///////////////////////////////////////////////////////////////////////////////
// Private variables
///////////////////////////////////////////////////////////////////////////////

// TODO: No hardcode
static struct i2cmst_priv *devs[2];

///////////////////////////////////////////////////////////////////////////////
// Helper functions
///////////////////////////////////////////////////////////////////////////////

///////////////////////////////////////////////////////////////////////////////
// Driver API implementation
///////////////////////////////////////////////////////////////////////////////

static int i2cmst_gr716_init(const struct device *dev)
{
    return i2cmst_autoinit();
}

static int i2cmst_gr716a_configure(const struct device *dev, uint32_t dev_config)
{
    struct i2cmst_gr716a_config *config = (struct i2cmst_gr716a_config *)(dev->config);
    int dev_no = config->dev_no;

    if (!(I2C_MODE_MASTER & dev_config))
        return -EINVAL;

    int speed = 0;
    switch (I2C_SPEED_GET(dev_config))
    {
    case I2C_SPEED_STANDARD:
        speed = KHZ(100);
        break;
    case I2C_SPEED_FAST:
        speed = KHZ(400);
        break;
    
    default:
        return -EINVAL;
    }
    
    int ten_bit_addr_enable = 0;
    if (I2C_MSG_ADDR_10_BITS & dev_config)
        ten_bit_addr_enable = 1;
    
    struct i2cmst_priv *i2cmst_dev = i2cmst_open(dev_no);
    if (!i2cmst_dev)
        return -1;
    
    int ret;
    ret = i2cmst_stop(i2cmst_dev);
    if (ret != DRV_OK)
        DEBUG(printk("Warning: i2cmst already stopped.\n");)
    
    ret = i2cmst_set_retries(i2cmst_dev, config->retries);
    if (ret != DRV_OK)
    {
        DEBUG(printk("Error: i2cmst_set_retries\n");)
        return ret;
    }

    ret = i2cmst_set_speed(i2cmst_dev, speed);
    if (ret != DRV_OK)
    {
        DEBUG(printk("Error: i2cmst_set_speed\n");)
        DEBUG(printk("Note: Speed must be either 100000 or 400000\n");)
        return ret;
    }

    ret = i2cmst_set_interrupt_mode(i2cmst_dev, config->interrupt_enable);
    if (ret != DRV_OK)
    {
        DEBUG(printk("Error: i2cmst_set_interrupt_mode\n");)
        return ret;
    }

    ret = i2cmst_set_ten_bit_addr(i2cmst_dev, ten_bit_addr_enable);
    if (ret != DRV_OK)
    {
        DEBUG(printk("Error: i2cmst_set_ten_bit_addr\n");)
        return ret;
    }

    i2cmst_clr_stats(i2cmst_dev);
    devs[dev_no] = i2cmst_dev;

    return 0;
}

// This function basically just translates between the library driver and
// the zephyr implementation
static int i2cmst_gr716a_transfer(const struct device *dev, struct i2c_msg *msgs,
                               uint8_t num_msgs, uint16_t addr)
{
    if (!num_msgs)
        return 0;
    
    for (int i = 0; i < num_msgs; i++)
    {
        uint32_t i2cmst_flags = 0;
        if ((msgs[i].flags & I2C_MSG_RW_MASK) == I2C_MSG_READ)
            i2cmst_flags |= I2CMST_FLAGS_READ;
        else
            i2cmst_flags |= !I2CMST_FLAGS_READ;
        
        int i2cmst_slave = addr;
        int i2cmst_length = msgs[i].len;
        uint8_t *i2cmst_payload = msgs[i].buf;

        struct i2cmst_packet pkt = {
            .flags = i2cmst_flags,
            .slave = i2cmst_slave,
            .length = i2cmst_length,
            .payload = i2cmst_payload
        };

        // TODO: Make this much more effective by creating one large
        // list and pass it all into i2cmst_request at once
        struct i2cmst_list single_list = {
            .head = &pkt,
            .tail = &pkt,
        };

        struct i2cmst_gr716a_config *config = (struct i2cmst_gr716a_config *)(dev->config);
        
        int dev_no = config->dev_no;
        struct i2cmst_priv *dev = devs[dev_no];
        int ret = i2cmst_request(dev, &single_list);
        if (ret != DRV_OK)
        {
            DEBUG(printk("Error: i2cmst_request\n");)
            return ret;
        }
    }

    return 0;
}

///////////////////////////////////////////////////////////////////////////////
// Driver API registration
///////////////////////////////////////////////////////////////////////////////

static const struct i2c_driver_api i2c_gr716_driver_api = {
    .configure = i2cmst_gr716a_configure,
    .transfer = i2cmst_gr716a_transfer,
};

static struct i2cmst_packet packet;
static struct i2cmst_gr716a_config config;

DEVICE_DT_DEFINE(
    DT_NODELABEL(i2c0),
    i2cmst_gr716_init,
    NULL,
    &packet,
    &config,
    POST_KERNEL,
    CONFIG_KERNEL_INIT_PRIORITY_DEVICE,
    &i2c_gr716_driver_api,
);

#if DT_NODE_HAS_STATUS(DT_NODELABEL(i2c0), okay)
// Fetch data from the DT_NODE into config struct using DT macros

#endif

#if DT_NODE_HAS_STATUS(DT_NODELABEL(i2c1), okay)
// Fetch data from the DT_NODE into config struct using DT macros

#endif