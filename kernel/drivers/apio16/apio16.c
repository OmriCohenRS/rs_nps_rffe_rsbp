// SPDX-License-Identifier: GPL-2.0
/*
 * APIO16 SPI 16‑bit I/O Expander Driver
 * Copyright (c) 2026 Ramon.Space
 */

#include <linux/module.h>
#include <linux/spi/spi.h>
#include <linux/of.h>
#include <linux/gpio/driver.h>
#include <linux/mutex.h>

#define APIO16_REG_IN0      0
#define APIO16_REG_IN1      1
#define APIO16_REG_OUT0     2
#define APIO16_REG_OUT1     3
#define APIO16_REG_POL0     4
#define APIO16_REG_POL1     5
#define APIO16_REG_CFG0     6
#define APIO16_REG_CFG1     7

#define APIO16_NUM_GPIOS    16

#define APIO16_SPI_RD(reg)  (((reg) << 5) | (0 << 4))
#define APIO16_SPI_WR(reg)  (((reg) << 5) | (1 << 4))

struct apio16 {
	struct spi_device *spi;
	struct gpio_chip gc;
	struct mutex lock;

	u8 reg_in[2];
	u8 reg_out[2];
	u8 reg_cfg[2];
};

/* SPI read register */
static int apio16_read_reg(struct apio16 *chip, u8 reg, u8 *val)
{
	u8 tx[2] = { APIO16_SPI_RD(reg), 0 };
	u8 rx[2] = {};
	struct spi_transfer t = {
		.tx_buf = tx,
		.rx_buf = rx,
		.len = 2,
	};
	int ret;

	ret = spi_sync_transfer(chip->spi, &t, 1);
	if (ret < 0)
		return ret;

	*val = rx[1];
	return 0;
}

/* SPI write register */
static int apio16_write_reg(struct apio16 *chip, u8 reg, u8 val)
{
	u8 tx[2] = { APIO16_SPI_WR(reg), val };
	struct spi_transfer t = {
		.tx_buf = tx,
		.len = 2,
	};

	return spi_sync_transfer(chip->spi, &t, 1);
}

/* Map GPIO offset → port + bit */
static inline void apio16_offset_to_portbit(unsigned int offset,
						unsigned int *port,
						unsigned int *bit)
{
	*port = offset / 8;
	*bit = offset % 8;
}

/* Get GPIO direction */
static int apio16_gpio_get_direction(struct gpio_chip *gc,
					unsigned int offset)
{
	struct apio16 *chip = gpiochip_get_data(gc);
	unsigned int port, bit;
	int dir;

	apio16_offset_to_portbit(offset, &port, &bit);

	mutex_lock(&chip->lock);
	dir = !!(chip->reg_cfg[port] & BIT(bit));
	mutex_unlock(&chip->lock);
	return dir;
}

/* Set direction = input */
static int apio16_gpio_direction_input(struct gpio_chip *gc,
					unsigned int offset)
{
	struct apio16 *chip = gpiochip_get_data(gc);
	unsigned int port, bit;

	apio16_offset_to_portbit(offset, &port, &bit);

	mutex_lock(&chip->lock);
	chip->reg_cfg[port] |= BIT(bit);
	apio16_write_reg(chip, APIO16_REG_CFG0 + port,
			 chip->reg_cfg[port]);
	mutex_unlock(&chip->lock);

	return 0;
}

/* Set direction = output and assign value */
static int apio16_gpio_direction_output(struct gpio_chip *gc,
					unsigned int offset, int value)
{
	struct apio16 *chip = gpiochip_get_data(gc);
	unsigned int port, bit;

	apio16_offset_to_portbit(offset, &port, &bit);

	mutex_lock(&chip->lock);

	chip->reg_cfg[port] &= ~BIT(bit);
	apio16_write_reg(chip, APIO16_REG_CFG0 + port,
			 chip->reg_cfg[port]);

	if (value)
		chip->reg_out[port] |= BIT(bit);
	else
		chip->reg_out[port] &= ~BIT(bit);

	apio16_write_reg(chip, APIO16_REG_OUT0 + port,
			 chip->reg_out[port]);

	mutex_unlock(&chip->lock);

	return 0;
}

/* Set output value */
static void apio16_gpio_set(struct gpio_chip *gc,
				unsigned int offset, int value)
{
	struct apio16 *chip = gpiochip_get_data(gc);
	unsigned int port, bit;

	apio16_offset_to_portbit(offset, &port, &bit);

	mutex_lock(&chip->lock);

	if (value)
		chip->reg_out[port] |= BIT(bit);
	else
		chip->reg_out[port] &= ~BIT(bit);

	apio16_write_reg(chip, APIO16_REG_OUT0 + port,
			 chip->reg_out[port]);

	mutex_unlock(&chip->lock);
}

/* Read input value */
static int apio16_gpio_get(struct gpio_chip *gc, unsigned int offset)
{
	struct apio16 *chip = gpiochip_get_data(gc);
	unsigned int port, bit;
	u8 val;

	apio16_offset_to_portbit(offset, &port, &bit);

	mutex_lock(&chip->lock);
	apio16_read_reg(chip, APIO16_REG_IN0 + port, &val);
	mutex_unlock(&chip->lock);

	return !!(val & BIT(bit));
}

/* Probe */
static int apio16_probe(struct spi_device *spi)
{
	struct apio16 *chip;
	u8 val;
	int ret;
	static const char * const apio16_names[16] = {
		"P0_0","P0_1","P0_2","P0_3","P0_4","P0_5","P0_6","P0_7",
		"P1_0","P1_1","P1_2","P1_3","P1_4","P1_5","P1_6","P1_7"
	};

	dev_info(&spi->dev, "APIO16 probe\n");

	chip = devm_kzalloc(&spi->dev, sizeof(*chip), GFP_KERNEL);
	if (!chip)
		return -ENOMEM;

	chip->spi = spi;
	spi_set_drvdata(spi, chip);
	spi->bits_per_word = 8;

	ret = spi_setup(spi);
	if (ret)
		return dev_err_probe(&spi->dev, ret, "spi_setup failed\n");

	/* Basic read test */
	ret = apio16_read_reg(chip, APIO16_REG_IN0, &val);
	if (ret)
		return dev_err_probe(&spi->dev, ret,
					 "Failed reading register 0\n");

	mutex_init(&chip->lock);

	chip->reg_out[0] = 0xFF;
	chip->reg_out[1] = 0xFF;
	chip->reg_cfg[0] = 0xFF;
	chip->reg_cfg[1] = 0xFF;

	chip->gc.label = dev_name(&spi->dev);
	chip->gc.parent = &spi->dev;
	chip->gc.owner = THIS_MODULE;
	chip->gc.base = -1;
	chip->gc.ngpio = APIO16_NUM_GPIOS;
	chip->gc.can_sleep = true;
	chip->gc.names = apio16_names;

	chip->gc.get_direction = apio16_gpio_get_direction;
	chip->gc.direction_input = apio16_gpio_direction_input;
	chip->gc.direction_output = apio16_gpio_direction_output;
	chip->gc.get = apio16_gpio_get;
	chip->gc.set = apio16_gpio_set;

	ret = devm_gpiochip_add_data(&spi->dev, &chip->gc, chip);
	if (ret)
		return dev_err_probe(&spi->dev, ret,
					"Failed to register GPIO controller\n");

	dev_info(&spi->dev,
		"APIO16 GPIO controller registered (%d lines)\n",
		APIO16_NUM_GPIOS);

	return 0;
}

static const struct of_device_id apio16_of_match[] = {
	{ .compatible = "ramon-space,apio16" },
	{ }
};
MODULE_DEVICE_TABLE(of, apio16_of_match);

static const struct spi_device_id apio16_id[] = {
	{ "apio16", 0 },
	{ }
};

static struct spi_driver apio16_driver = {
	.driver = {
		.name        = "apio16",
		.of_match_table = apio16_of_match,
	},
	.probe    = apio16_probe,
	.id_table = apio16_id,
};

module_spi_driver(apio16_driver);

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION("APIO16 16-bit SPI I/O expander driver");
MODULE_AUTHOR("Omri Cohen");