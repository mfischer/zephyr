/*
 * Copyright (c) 2022 Espressif Systems (Shanghai) Co., Ltd.
 *
 * SPDX-License-Identifier: Apache-2.0
 */

#define DT_DRV_COMPAT espressif_esp32_dac

#include <soc.h>
#include <zephyr/device.h>
#include <zephyr/drivers/dac.h>
#include <zephyr/drivers/clock_control.h>
#include <hal/rtc_io_types.h>
#include <hal/rtc_io_hal.h>
#include <hal/rtc_io_ll.h>
#include <hal/dac_hal.h>
#include <hal/dac_types.h>
#include "driver/dac_common.h"

#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(esp32_dac, CONFIG_DAC_LOG_LEVEL);

struct dac_esp32_config {
	int irq_source;
	struct clk *clk;
};

static int dac_esp32_write_value(const struct device *dev,
				uint8_t channel, uint32_t value)
{
	ARG_UNUSED(dev);

	dac_output_voltage(channel, value);

	return 0;
}

static int dac_esp32_channel_setup(const struct device *dev,
		   const struct dac_channel_cfg *channel_cfg)
{
	ARG_UNUSED(dev);

	if (channel_cfg->channel_id > DAC_CHANNEL_MAX) {
		LOG_ERR("Channel %d is not valid", channel_cfg->channel_id);
		return -EINVAL;
	}

	dac_output_enable(channel_cfg->channel_id);

	return 0;
}

static int dac_esp32_init(const struct device *dev)
{
	const struct dac_esp32_config *cfg = dev->config;

	if (!cfg->clk) {
		LOG_ERR("Clock device missing");
		return -EINVAL;
	}

	if (!device_is_ready(cfg->clk.dev)) {
		LOG_ERR("Clock device not ready");
		return -ENODEV;
	}

	if (clock_control_on(&cfg->clk) != 0) {
		LOG_ERR("DAC clock setup failed (%d)", -EIO);
		return -EIO;
	}

	return 0;
}

static const struct dac_driver_api dac_esp32_driver_api = {
	.channel_setup = dac_esp32_channel_setup,
	.write_value = dac_esp32_write_value
};

#define ESP32_DAC_INIT(id)									\
												\
	static const struct dac_esp32_config dac_esp32_config_##id = {				\
		.irq_source = DT_INST_IRQN(id),							\
		.clk = DT_INST_CLOCKS_GET_CLK_BY_IDX(id, 0),                                    \
	};											\
												\
	DEVICE_DT_INST_DEFINE(id,								\
		&dac_esp32_init,								\
		NULL,										\
		NULL,										\
		&dac_esp32_config_##id,								\
		POST_KERNEL,									\
		CONFIG_DAC_INIT_PRIORITY,							\
		&dac_esp32_driver_api);

DT_INST_FOREACH_STATUS_OKAY(ESP32_DAC_INIT);
