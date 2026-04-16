/* SPDX-License-Identifier: GPL-2.0
 * AFE11612 adc over spi
 * Copyright (c) 2026 Ramon.Space
 */

#include <linux/module.h>
#include <linux/of.h>
#include <linux/spi/spi.h>
#include <linux/mutex.h>
#include <linux/delay.h>
#include <linux/iio/iio.h>
#include <linux/iio/sysfs.h>
#include <linux/bitfield.h>
#include <linux/iopoll.h>

/* Register map */
#define AFE_REG_TEMP_LOCAL          0x00    /* Local (internal) temperature sensor register */
#define AFE_REG_TEMP_REMOTE1        0x01    /* Remote temperature sensor 1 */
#define AFE_REG_TEMP_REMOTE2        0x02    /* Remote temperature sensor 2 */
#define AFE_REG_ADC_DATA_BASE       0x23    /* Base address for ADC conversion */
#define AFE_REG_ADC_CH_EN0          0x50    /* ADC Channels 0-12 Enable */
#define AFE_REG_ADC_CH_EN1          0x51    /* ADC Channels 13-15 Enable */
#define AFE_REG_CFG                 0x4C    /* Main configuration register */
#define AFE_REG_PWR_CFG             0x6B    /* Power configuration register */
#define AFE_REG_DEVICE_ID           0x6C    /* Device ID register (read-only) */
#define AFE_REG_SW_RESET            0x7C    /* Software reset register */

/* Register bit definitions */
/* regs 0x50 and 0x51*/
#define AFE_REG_ADC_CH_EN0_DEFAULT  0x6DFF  /* Enable channels 0-12 */
#define AFE_REG_ADC_CH_EN1_DEFAULT  0x7000  /* Enable channels 13-15 */
/* reg 0x4C */ 
#define AFE_ADC_DATA_READY          BIT(7)  /* Data ready to read */
#define AFE_ADC_REF_INT             BIT(10) /* Enable internal reference adc */
#define AFE_ADC_INT_CONV            BIT(12) /* Start conversion */
#define AFE_ADC_CTRL_CFG_DEFAULT    (AFE_ADC_REF_INT)
#define AFE_ADC_MEAS                (AFE_ADC_REF_INT | AFE_ADC_INT_CONV)
/* reg 0x6B */ 
#define AFE_PWR_ADC                 BIT(13) /* Power-down mode control bit */
#define AFE_PWR_REF                 BIT(14) /* Internal reference in power-down mode control bit */
#define AFE_PWR_CFG                 (AFE_PWR_ADC | AFE_PWR_REF)  
/* reg 0x6C */ 
#define AFE_DEVICE_ID_EXPECTED      0x1220
/* reg 0x7C */ 
#define AFE_SW_RESET                0x6600  /* Fixed magic number for reset */

/* Additional afe11612 definitions*/
#define AFE_TEMP_FIELD        GENMASK(15, 4)

#define AFE_VREF     2.5     /* ADC reference voltage (V) */
#define AFE_EXT_DIV  2.0     /* External divider ratio */
#define AFE_ADC_MAX  4095.0  /* 12-bit ADC max value */

#define AFE_CMD_WRITE          0x00
#define AFE_CMD_READ           0x80
#define AFE_CMD(op, reg)       ((op) | ((reg) & 0x7F))

/* ADC conversion timing */
#define AFE_ADC_TIMEOUT_US     20000
#define AFE_ADC_POLL_US        50

struct afe11612_state {
	struct spi_device   *spi;
	struct mutex        lock;
	struct iio_dev      *indio_dev;
};

static int afe11612_spi_write_reg(struct afe11612_state *st,
								u8 reg, u16 val)
{
	int ret;
	u8 tx[3];

	struct spi_transfer t = {
		.tx_buf = tx,
		.len = sizeof(tx),
	};

	tx[0] = AFE_CMD(AFE_CMD_WRITE, reg);
	tx[1] = val >> 8;
	tx[2] = val & 0xff;

	ret = spi_sync_transfer(st->spi, &t, 1);
	if (ret < 0)
		return ret;

	return 0;
}

static int afe11612_spi_read_reg(struct afe11612_state *st,
								u8 reg, u16 *val)
{
	int ret;
	u8 tx[3] = {};
	u8 rx[3] = {};

	struct spi_transfer t = {
		.tx_buf = tx,
		.rx_buf = rx,
		.len = sizeof(tx),
	};
	
	tx[0] = AFE_CMD(AFE_CMD_READ, reg);

	spi_sync_transfer(st->spi, &t, 1); /* dummy */
	ret = spi_sync_transfer(st->spi, &t, 1);
	if (ret < 0) {
		return ret;
	}

	*val = (rx[1] << 8) | rx[2];

	return 0;
}

static u16 afe11612_read_device_id(struct afe11612_state *st)
{
	u16 id;

	mutex_lock(&st->lock);
	afe11612_spi_read_reg(st, AFE_REG_DEVICE_ID, &id);
	mutex_unlock(&st->lock);
	
	return id;
}

static int afe11612_hw_init(struct afe11612_state *st)
{
	int ret = 0;

	mutex_lock(&st->lock);

	ret = afe11612_spi_write_reg(st, AFE_REG_SW_RESET,
					 AFE_SW_RESET);
	if (ret)
		goto out;

	usleep_range(1000, 2000);

	ret = afe11612_spi_write_reg(st, AFE_REG_PWR_CFG,
					 AFE_PWR_CFG);
	if (ret)
		goto out;

	ret = afe11612_spi_write_reg(st, AFE_REG_CFG,
					 AFE_ADC_CTRL_CFG_DEFAULT);
	if (ret)
		goto out;

	ret = afe11612_spi_write_reg(st, AFE_REG_ADC_CH_EN0,
					AFE_REG_ADC_CH_EN0_DEFAULT);
	if (ret)
		goto out;

	ret = afe11612_spi_write_reg(st, AFE_REG_ADC_CH_EN1,
					AFE_REG_ADC_CH_EN1_DEFAULT);
	if (ret)
		goto out;

	ret = afe11612_spi_write_reg(st, AFE_REG_CFG,
					AFE_ADC_MEAS);
	if (ret)
		goto out;

	usleep_range(1000, 2000);

out:
	mutex_unlock(&st->lock);
	return ret;
}

static int afe11612_wait_data_ready(struct afe11612_state *st)
{
    u16 val;
    int ret;

    ret = read_poll_timeout(afe11612_spi_read_reg,
                            ret,
                            (ret == 0) && (val & AFE_ADC_DATA_READY),
                            AFE_ADC_POLL_US,
                            AFE_ADC_TIMEOUT_US,
                            false,
                            st, AFE_REG_CFG, &val);

    return ret;
}

static int afe11612_read_raw(struct iio_dev *indio_dev,
							const struct iio_chan_spec *chan,
							int *val, int *val2,
							long mask)
{
	struct afe11612_state *st = iio_priv(indio_dev);
	u16 raw;
	int reg, ret = 0;

	mutex_lock(&st->lock);

	ret = afe11612_spi_write_reg(st, AFE_REG_CFG, AFE_ADC_MEAS);
	if (ret)
		goto out_unlock;

	ret = afe11612_wait_data_ready(st);
	if (ret)
		goto out_unlock;

	if (chan->type == IIO_VOLTAGE)
		reg = AFE_REG_ADC_DATA_BASE + chan->channel;
	else
		reg = chan->address;

	ret = afe11612_spi_read_reg(st, reg, &raw);
	if (ret)
		goto out_unlock;

	switch (mask) {
	case IIO_CHAN_INFO_RAW:
		if (chan->type == IIO_TEMP)
			*val = FIELD_GET(AFE_TEMP_FIELD, raw);
		else
			*val = raw;
		ret = IIO_VAL_INT;
		break;

	case IIO_CHAN_INFO_PROCESSED:
		if (chan->type == IIO_TEMP) {
			int t;

			t = FIELD_GET(AFE_TEMP_FIELD, raw) * 125;
			*val = t / 1000;
			*val2 = t % 1000;
			ret = IIO_VAL_INT_PLUS_MICRO;

		} else {
			s64 mv;

			mv = raw;
			mv *= (s64)(AFE_VREF * AFE_EXT_DIV);
			mv = div64_s64(mv, AFE_ADC_MAX);

			*val  = mv / 1000;
			*val2 = mv % 1000;
			ret = IIO_VAL_INT_PLUS_MICRO;
		}
		break;

	default:
		ret = -EINVAL;
	}

out_unlock:
	mutex_unlock(&st->lock);
	return ret;
}

#define AFE_ADC_CHAN(_idx) {            \
	.type = IIO_VOLTAGE,                \
	.indexed = 1,                       \
	.channel = (_idx),                  \
	.info_mask_separate =               \
		BIT(IIO_CHAN_INFO_RAW) |        \
		BIT(IIO_CHAN_INFO_PROCESSED),   \
}

#define AFE_TEMP_CHAN(_idx, _reg) {     \
	.type = IIO_TEMP,                   \
	.indexed = 1,                       \
	.channel = (_idx),                  \
	.address = (_reg),                  \
	.info_mask_separate =               \
		BIT(IIO_CHAN_INFO_RAW) |        \
		BIT(IIO_CHAN_INFO_PROCESSED),   \
}

static const struct iio_chan_spec afe11612_channels[] = {
	AFE_ADC_CHAN(0),  AFE_ADC_CHAN(1),
	AFE_ADC_CHAN(2),  AFE_ADC_CHAN(3),
	AFE_ADC_CHAN(4),  AFE_ADC_CHAN(5),
	AFE_ADC_CHAN(6),  AFE_ADC_CHAN(7),
	AFE_ADC_CHAN(8),  AFE_ADC_CHAN(9),
	AFE_ADC_CHAN(10), AFE_ADC_CHAN(11),
	AFE_ADC_CHAN(12), AFE_ADC_CHAN(13),
	AFE_ADC_CHAN(14), AFE_ADC_CHAN(15),

	AFE_TEMP_CHAN(0, AFE_REG_TEMP_LOCAL),
	AFE_TEMP_CHAN(1, AFE_REG_TEMP_REMOTE1),
	AFE_TEMP_CHAN(2, AFE_REG_TEMP_REMOTE2),
};

static const struct iio_info afe11612_iio_info = {
	.read_raw = afe11612_read_raw,
};

//TODO
static ssize_t sw_reset_store(struct device *dev,
                             struct device_attribute *attr,
                             const char *buf, size_t count)
{
    struct spi_device *spi = to_spi_device(dev);
    struct iio_dev *indio_dev = spi_get_drvdata(spi);
    struct afe11612_state *st;
    int ret, val;

    if (!indio_dev)
        return -ENODEV;

    st = iio_priv(indio_dev);

    if (!st)
        return -ENODEV;

    if (kstrtoint(buf, 10, &val))
        return -EINVAL;

    if (val != 1)
        return -EINVAL;

    dev_info(dev, "Software reset triggered\n");

    ret = afe11612_hw_init(st);
    if (ret)
        return ret;

    return count;
}

static DEVICE_ATTR_WO(sw_reset);

static ssize_t device_id_show(struct device *dev,
							struct device_attribute *attr,
							char *buf)
{
	u16 id;

	struct spi_device *spi = to_spi_device(dev);
	struct iio_dev *indio_dev = spi_get_drvdata(spi);
	struct afe11612_state *st = iio_priv(indio_dev);

	id = afe11612_read_device_id(st);

	return sysfs_emit(buf, "0x%04x\n", id);
}

static DEVICE_ATTR_RO(device_id);

static struct attribute *afe11612_dev_attrs[] = {
	&dev_attr_device_id.attr,
	&dev_attr_sw_reset.attr,
	NULL,
};

static const struct attribute_group afe11612_dev_attr_group = {
	.attrs = afe11612_dev_attrs,
};

static int afe11612_probe(struct spi_device *spi)
{
	struct iio_dev *indio_dev;
	struct afe11612_state *st;
	int ret;
	u16 id;

	indio_dev = devm_iio_device_alloc(&spi->dev, sizeof(*st));
	if (!indio_dev)
		return -ENOMEM;

	dev_info(&spi->dev, "AFE11612 IIO probe\n");

	st = iio_priv(indio_dev);
	st->spi = spi;
	st->indio_dev = indio_dev;
	mutex_init(&st->lock);
	spi->bits_per_word = 8;

	ret = spi_setup(spi);
	if (ret) {
		dev_err(&spi->dev, "SPI setup failed\n");
		return ret;
	}

	ret = afe11612_hw_init(st);
	if (ret) {
		dev_err(&spi->dev, "AFE11612 failed to init device\n");
		return ret;
	}

	id = afe11612_read_device_id(st);
	dev_info(&st->spi->dev, "AFE11612 IIO device id: 0x%x\n", id);

	indio_dev->name = "afe11612";
	indio_dev->modes = INDIO_DIRECT_MODE;
	indio_dev->info = &afe11612_iio_info;
	indio_dev->channels = afe11612_channels;
	indio_dev->num_channels = ARRAY_SIZE(afe11612_channels);

	spi_set_drvdata(spi, indio_dev);

	ret = devm_iio_device_register(&spi->dev, indio_dev);
	if (ret)
		return ret;

	ret = devm_device_add_group(&spi->dev, &afe11612_dev_attr_group);
	if (ret)
		return ret;

	dev_info(&spi->dev, "AFE11612 IIO device registered\n");

	return 0;
}

static const struct of_device_id afe11612_of_match[] = {
	{ .compatible = "ti,afe11612" },
	{ }
};
MODULE_DEVICE_TABLE(of, afe11612_of_match);

static const struct spi_device_id afe11612_id[] = {
	{ "afe11612", 0 },
	{ }
};

static struct spi_driver afe11612_driver = {
	.driver = {
		.name = "afe11612",
		.of_match_table = afe11612_of_match,
	},
	.probe = afe11612_probe,
	.id_table = afe11612_id,
};

module_spi_driver(afe11612_driver);

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Omri Cohen");
MODULE_DESCRIPTION("AFE11612 SPI ADC & Temperature Driver");